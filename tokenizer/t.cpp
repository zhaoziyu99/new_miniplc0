#include "tokenizer/tokenizer.h"
#include<string>
#include <cctype>
#include <sstream>
using namespace std;
namespace miniplc0 {

	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
		if (!_initialized)
			readAll();
		if (_rdr.bad())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
		if (isEOF())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
		auto p = nextToken();
		if (p.second.has_value())
			return std::make_pair(p.first, p.second);
		auto err = checkToken(p.first.value());
		if (err.has_value())
			return std::make_pair(p.first, err.value());
		return std::make_pair(p.first, std::optional<CompilationError>());
	}

	std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
		std::vector<Token> result;
		while (true) {
			auto p = NextToken();
			if (p.second.has_value()) {
				if (p.second.value().GetCode() == ErrorCode::ErrEOF)
					return std::make_pair(result, std::optional<CompilationError>());
				else
					return std::make_pair(std::vector<Token>(), p.second);
			}
			result.emplace_back(p.first.value());
		}
	}

	// ע�⣺����ķ���ֵ�� Token �� CompilationError ֻ�ܷ���һ��������ͬʱ���ء�
	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
		// ���ڴ洢�Ѿ���������ɵ�ǰtoken�ַ�
		std::stringstream ss;
		// ����token�Ľ������Ϊ�˺����ķ���ֵ
		std::pair<std::optional<Token>, std::optional<CompilationError>> result;
		// <�кţ��к�>����ʾ��ǰtoken�ĵ�һ���ַ���Դ�����е�λ��
		std::pair<int64_t, int64_t> pos;
		// ��¼��ǰ�Զ�����״̬������˺���ʱ�ǳ�ʼ״̬
		DFAState current_state = DFAState::INITIAL_STATE;
		// ����һ����ѭ����������������
		// ÿһ��ִ��while�ڵĴ��룬�����ܵ���״̬�ı��
		while (true) {
			// ��һ���ַ�����ע��auto�Ƶ��ó���������std::optional<char>
			// ������ʵ������д��
			// 1. ÿ��ѭ��ǰ��������һ�� char
			// 2. ֻ���ڿ��ܻ�ת�Ƶ�״̬����һ�� char
			// ��Ϊ����ʵ���� unread��Ϊ��ʡ������ѡ���һ��
			auto current_char = nextChar();
			// ��Ե�ǰ��״̬���в�ͬ�Ĳ���
			switch (current_state) {

				// ��ʼ״̬
				// ��� case ���Ǹ����˺����߼������Ǻ���� case �����հᡣ
			case INITIAL_STATE: {
				// �Ѿ��������ļ�β
				if (!current_char.has_value())
					// ����һ���յ�token���ͱ������ErrEOF���������ļ�β
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrEOF));

				// ��ȡ�������ַ���ֵ��ע��auto�Ƶ�����������char
				auto ch = current_char.value();
				// ����Ƿ�����˲��Ϸ����ַ�����ʼ��Ϊ��
				auto invalid = false;

				// ʹ�����Լ���װ���ж��ַ����͵ĺ����������� tokenizer/utils.hpp
				// see https://en.cppreference.com/w/cpp/string/byte/isblank
				if (miniplc0::isspace(ch)) // �������ַ��ǿհ��ַ����ո񡢻��С��Ʊ���ȣ�
					current_state = DFAState::INITIAL_STATE; // ������ǰ״̬Ϊ��ʼ״̬���˴�ֱ��breakҲ�ǿ��Ե�
				else if (!miniplc0::isprint(ch)) // control codes and backspace
					invalid = true;
				else if (miniplc0::isdigit(ch)) // �������ַ�������
					current_state = DFAState::UNSIGNED_INTEGER_STATE; // �л����޷���������״̬
				else if (miniplc0::isalpha(ch)) // �������ַ���Ӣ����ĸ
					current_state = DFAState::IDENTIFIER_STATE; // �л�����ʶ����״̬
				else {
					switch (ch) {
					case '=': // ����������ַ���`=`�����л������ںŵ�״̬
						current_state = DFAState::EQUAL_SIGN_STATE;
						break;
					case '-':
						// ����գ��л������ŵ�״̬
						current_state = DFAState::MINUS_SIGN_STATE;
						break;
					case '+':
						// ����գ��л����Ӻŵ�״̬
						current_state = DFAState::PLUS_SIGN_STATE;
						break;
					case '*':
						// ����գ��л�״̬
						current_state = DFAState::MULTIPLICATION_SIGN_STATE;
						break;
					case '/':
						// ����գ��л�״̬
						current_state = DFAState::DIVISION_SIGN_STATE;
						break;
						///// ����գ�
						///// ���������Ŀɽ����ַ�
						///// �л�����Ӧ��״̬
					case ';':
						current_state = DFAState::SEMICOLON_STATE;
						break;
					case '(':
						current_state = DFAState::LEFTBRACKET_STATE;
						break;
					case ')':
						current_state = DFAState::RIGHTBRACKET_STATE;
						break;
						// �����ܵ��ַ����µĲ��Ϸ���״̬
					default:
						invalid = true;
						break;
					}
				}
				// ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
				if (current_state != DFAState::INITIAL_STATE)
					pos = previousPos(); // ��¼���ַ��ĵ�λ��Ϊtoken�Ŀ�ʼλ��
				// �����˲��Ϸ����ַ�
				if (invalid) {
					// ��������ַ�
					unreadLast();
					// ���ر�����󣺷Ƿ�������
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// ����������ַ�������״̬��ת�ƣ�˵������һ��token�ĵ�һ���ַ�
				if (current_state != DFAState::INITIAL_STATE) // ignore white spaces
					ss << ch; // �洢�������ַ�
				break;
			}

							  // ��ǰ״̬���޷�������
			case UNSIGNED_INTEGER_STATE: {
				// ����գ�
				// �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���Ϊ����
				if (!current_char.has_value()) {
					//�����ɹ��򷵻��޷����������͵�token�����򷵻ر������
					unsigned long long num;
					string str = ss.str();
					int len = len;
					if (len > 10)//����int��Χ,����Խ�����
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrIntegerOverflow));
					else if (len == 10)
					{
						ss >> num;
						if (num > INT_MAX)//����int��Χ,����Խ�����
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrIntegerOverflow));
					}
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, num, pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				// ����������ַ������֣���洢�������ַ�
				if (miniplc0::isdigit(ch))
					ss << ch;
				// �������������ĸ����洢�������ַ������л�״̬����ʶ��
				else if (miniplc0::isalpha(ch)) {
					ss << ch;
					current_state = DFAState::IDENTIFIER_STATE;
				}
				// ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���Ϊ����
				//     �����ɹ��򷵻��޷����������͵�token�����򷵻ر������
				else {
					unreadLast();
					unsigned long long num;
					string str;
					str = ss.str();
					int len = len;
					if (len > 10)//����int��Χ,����Խ�����
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrIntegerOverflow));
					else if (len == 10)
					{
						ss >> num;
						if (num > INT_MAX)//����int��Χ,����Խ�����
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrIntegerOverflow));
					}
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, num, pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}
			case IDENTIFIER_STATE: {
				// ����գ�
				// �����ǰ�Ѿ��������ļ�β��������Ѿ��������ַ���
				//�����ɹ��򷵻��޷��ű�ʶ�����͵�token�����򷵻ر������

				if (!current_char.has_value()) {
					string str;
					str = ss.str();
					int len = len;
					if (!miniplc0::isalpha(str.at(0)))
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrInvalidIdentifier));
					// �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token
					if (str == "begin")
						return std::make_pair(std::make_optional<Token>(TokenType::BEGIN, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "end")
						return std::make_pair(std::make_optional<Token>(TokenType::END, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "const")
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "var")
						return std::make_pair(std::make_optional<Token>(TokenType::VAR, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "print")
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, str, pos, currentPos()), std::optional<CompilationError>());

					return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, str, pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				// ������������ַ�����ĸ����洢�������ַ�
				if (miniplc0::isalpha(ch))
					ss << ch;
				// ����������ַ������������֮һ������˶������ַ����������Ѿ��������ַ���
				else
				{
					unreadLast();
					string str;
					str = ss.str();
					int len = len;
					if (!miniplc0::isalpha(str.at(0)))
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrInvalidIdentifier));
					// �����������ǹؼ��֣���ô���ض�Ӧ�ؼ��ֵ�token�����򷵻ر�ʶ����token
					if (str == "begin")
						return std::make_pair(std::make_optional<Token>(TokenType::BEGIN, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "end")
						return std::make_pair(std::make_optional<Token>(TokenType::END, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "const")
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "var")
						return std::make_pair(std::make_optional<Token>(TokenType::VAR, str, pos, currentPos()), std::optional<CompilationError>());
					else if (str == "print")
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, str, pos, currentPos()), std::optional<CompilationError>());
					return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, str, pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}

								 // �����ǰ״̬�ǼӺ�
			case PLUS_SIGN_STATE: {
				// ��˼������ΪʲôҪ���ˣ��������ط��᲻����Ҫ
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, '+', pos, currentPos()), std::optional<CompilationError>());
			}
								// ��ǰ״̬Ϊ���ŵ�״̬
			case MINUS_SIGN_STATE: {
				// ����գ����ˣ������ؼ���token
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, '-', pos, currentPos()), std::optional<CompilationError>());
			}
			case MULTIPLICATION_SIGN_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, '*', pos, currentPos()), std::optional<CompilationError>());
			}
			case DIVISION_SIGN_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, '/', pos, currentPos()), std::optional<CompilationError>());
			}
			case EQUAL_SIGN_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, '=', pos, currentPos()), std::optional<CompilationError>());
			}
			case SEMICOLON_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, ';', pos, currentPos()), std::optional<CompilationError>());
			}
			case LEFTBRACKET_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, '(', pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBRACKET_STATE: {
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, ')', pos, currentPos()), std::optional<CompilationError>());
			}
								   // ����գ�
								   // ���������ĺϷ�״̬�����к��ʵĲ���
								   // ������н���������token�����ر������

								   // Ԥ��֮���״̬�����ִ�е������˵�������쳣
			default:
				DieAndPrint("unhandled state.");
				break;
			}
		}
		// Ԥ��֮���״̬�����ִ�е������˵�������쳣
		return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
	}

	std::optional<CompilationError> Tokenizer::checkToken(const Token& t) {
		switch (t.GetType()) {
		case IDENTIFIER: {
			auto val = t.GetValueString();
			if (miniplc0::isdigit(val[0]))
				return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second, ErrorCode::ErrInvalidIdentifier);
			break;
		}
		default:
			break;
		}
		return {};
	}

	void Tokenizer::readAll() {
		if (_initialized)
			return;
		for (std::string tp; std::getline(_rdr, tp);)
			_lines_buffer.emplace_back(std::move(tp + "\n"));
		_initialized = true;
		_ptr = std::make_pair<int64_t, int64_t>(0, 0);
		return;
	}

	// Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
	std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
		if (_ptr.first >= _lines_buffer.size())
			DieAndPrint("advance after EOF");
		if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
			return std::make_pair(_ptr.first + 1, 0);
		else
			return std::make_pair(_ptr.first, _ptr.second + 1);
	}

	std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
		return _ptr;
	}

	std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
		if (_ptr.first == 0 && _ptr.second == 0)
			DieAndPrint("previous position from beginning");
		if (_ptr.second == 0)
			return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
		else
			return std::make_pair(_ptr.first, _ptr.second - 1);
	}

	std::optional<char> Tokenizer::nextChar() {
		if (isEOF())
			return {}; // EOF
		auto result = _lines_buffer[_ptr.first][_ptr.second];
		_ptr = nextPos();
		return result;
	}

	bool Tokenizer::isEOF() {
		return _ptr.first >= _lines_buffer.size();
	}

	// Note: Is it evil to unread a buffer?
	void Tokenizer::unreadLast() {
		_ptr = previousPos();
	}
}