#include "json.h"

#include <cwctype>
#include <array>
#include <iomanip>

using namespace std;

namespace json {
    namespace {
        using Number = std::variant<int, double>;

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;
            for (char c; input >> c;) {
                if (c == ']') {
                    return Node(move(result));
                }
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            throw json::ParsingError("Array parsing error"s);
        }

        Number LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            if (input.peek() == '0') {
                read_char();
            } else {
                read_digits();
            }

            bool is_int = true;
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return std::stoi(parsed_num);
                    } catch (...) {
                    }
                }
                return std::stod(std::string(parsed_num).c_str());
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadString(istream& input) {
            string line;
            for (char c; input >> std::noskipws >> c;) {
                if (c == '"') {
                    input >> std::skipws;
                    return Node(move(line));
                }
                if (c == '\\') {
                    input >> c;
                    switch (c) {
                    case '"': line.push_back('"'); break;
                    case 'n': line.push_back('\n'); break;
                    case 'r': line.push_back('\r'); break;
                    case '\\': line.push_back('\\'); break;
                    case 't': line.push_back('\t'); break;
                    default:
                        input >> std::skipws;
                        throw json::ParsingError("bad string"s);
                    }
                } else {
                    line.push_back(c);
                }
            }
            throw json::ParsingError("String parsing error"s);
        }

        Node LoadDict(istream& input) {
            Dict result;
            for (char c; input >> c;) {
                if (c == '}') {
                    return Node(move(result));
                }
                if (c == ',') {
                    input >> c;
                }
                string key = LoadString(input).AsString();
                input >> c;
                result.insert({ move(key), LoadNode(input) });
            }
            throw json::ParsingError("Map parsing error"s);
        }

        Node LoadBool(istream& input) {
            static const string_view true_sv = "true"sv;
            static const string_view false_sv = "false"sv;
            size_t char_count = 0;
            size_t true_count = 0;
            size_t false_count = 0;
            for (char c; input >> c;) {
                ++char_count;
                if (c == false_sv[char_count - 1]) {
                    ++false_count;
                    if (false_count == false_sv.size()) {
                        if (std::iswalnum(input.peek())) {
                            throw ParsingError("Fail"s);
                        }
                        return Node(false);
                    }
                    continue;
                }
                if (c == true_sv[char_count - 1]) {
                    ++true_count;
                    if (true_count == true_sv.size()) {
                        if (std::iswalnum(input.peek())) {
                            throw ParsingError("Fail"s);
                        }
                        return Node(true);
                    }
                    continue;
                }
                throw json::ParsingError("bool type error"s);
            }
            throw json::ParsingError("bool type error"s);
        }

        Node LoadNull(istream& input) {
            static const string_view null_sv = "null"sv;
            size_t char_count = 0;
            size_t null_count = 0;
            for (char c; input >> c;) {
                ++char_count;
                if (c == null_sv[char_count - 1]) {
                    ++null_count;
                    if (null_count == null_sv.size()) {
                        return Node(nullptr);
                    }
                    continue;
                }
                throw json::ParsingError("null type error"s);
            }
            throw json::ParsingError("null type error"s);
        }

        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            } else if (c == '{') {
                return LoadDict(input);
            } else if (c == '"') {
                return LoadString(input);
            } else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            } else if (c == 'n') {
                input.putback(c);
                return LoadNull(input);
            } else {
                input.putback(c);
                Number x = LoadNumber(input);
                return (std::holds_alternative<int>(x) ? Node(std::get<int>(x)) : Node(std::get<double>(x)));
            }
        }

    } //namespace

    Node::Node(bool val) : data_(val) {}

    Node::Node(int val) : data_(val) {}

    Node::Node(double val) : data_(val) {}

    Node::Node(std::string val) : data_(std::move(val)) {}

    Node::Node(Array array) : data_(std::move(array)) {}

    Node::Node(Dict map) : data_(std::move(map)) {}

    bool Node::AsBool() const {
        if (std::holds_alternative<bool>(data_)) {
            return get<bool>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    int Node::AsInt() const {
        if (std::holds_alternative<int>(data_)) {
            return get<int>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    double Node::AsDouble() const {
        if (std::holds_alternative<double>(data_)) {
            return get<double>(data_);
        } else if (std::holds_alternative<int>(data_)) {
            return get<int>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    const string& Node::AsString() const {
        if (std::holds_alternative<std::string>(data_)) {
            return get<std::string>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    const Array& Node::AsArray() const {
        if (std::holds_alternative<Array>(data_)) {
            return get<Array>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    const Dict& Node::AsMap() const {
        if (std::holds_alternative<Dict>(data_)) {
            return get<Dict>(data_);
        } else {
            throw std::logic_error(""s);
        }
    }

    bool Node::IsNull() const {
        return (this->data_ == Data());
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(data_);
    }

    bool Node::IsInt() const {
        return std::holds_alternative<int>(data_);
    }

    bool Node::IsDouble() const {
        return this->IsPureDouble() ? std::holds_alternative<double>(data_) : std::holds_alternative<int>(data_);
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(data_);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(data_);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(data_);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(data_);
    }

    Data Node::Content() const {
        return this->data_;
    }

    Data* Node::Content_ptr() {
        Data* ptr = &this->data_;
        return ptr;
    }

    bool Node::operator==(const Node& other) const {
        return this->data_ == other.data_;
    }

    bool Node::operator!=(const Node& other) const {
        return this->data_ != other.data_;
    }

    Document::Document(Node root) : root_(move(root)) {}

    const Node& Document::GetRoot() const {
        return this->root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    struct VariantPrinter {
        ostream& out;
        void operator()(std::nullptr_t) const {
            out << "null"sv;
        }
        void operator()(Array array) const {
            out << "["sv;
            bool first = true;
            for (Node& n : array) {
                if (first) {
                    first = false;
                } else {
                    out << ',';
                }
                out << n.Content();
            }
            out << "]"sv;
        }
        void operator()(Dict map) const {
            out << "{"sv;
            bool first = true;
            for (auto& [key, value] : map) {
                if (first) {
                    first = false;
                } else {
                    out << ',';
                }
                out << "\""sv << key << "\": "sv << value.Content();
            }
            out << "}"sv;
        }
        void operator()(bool value) const {
            out << (value ? "true"sv : "false"sv);
        }
        void operator()(int value) const {
            out << value;
        }
        void operator()(double value) const {
            out << value;
        }
        void operator()(std::string value) const {
            out << "\""sv;
            for (char c : value) {
                switch (c) {
                case '"': out << "\\\""sv; break;
                case '\n': out << "\\n"sv; break;
                case '\r': out << "\\r"sv; break;
                case '\\': out << "\\\\"sv; break;
                case '\t': out << "\\t"sv; break;
                default:
                    out << c;
                }
            }
            out << "\""sv;
        }

    };

    ostream& operator<<(ostream& out, const Data& d) {
        visit(VariantPrinter{ out }, d);
        return out;
    }

    ostream& operator<<(ostream& out, const Node& d) {
        visit(VariantPrinter{ out }, d.Content());
        return out;
    }

    void Print(const Document& doc, std::ostream& output) {
        visit(VariantPrinter{ output }, doc.GetRoot().Content());
    }
} //namespace jso