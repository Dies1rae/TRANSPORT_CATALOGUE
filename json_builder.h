#pragma once

#include "json.h"

#include <memory>
#include <stack>
#include <string>
#include <exception>

namespace json {
	class DictItemContext;
	class ArrayItemContext;
	class ValueItemContext;

	class Builder {
	public:
		Builder() {}
		~Builder() {
			this->Clean_all();
		}

		ValueItemContext Key(const std::string& key);
		ValueItemContext Key(std::string&& key);
		DictItemContext StartDict();
		ArrayItemContext StartArray();

		Builder& Value(Data&& value);
		Builder& Value(const Data& value);
		Builder& EndDict();
		Builder& EndArray();

		void Clean_all();
		Node&& Build();
		bool IsDictKeyTop();
	private:
		enum class state { EMPTY, EDITION, READY };
		state state_ = state::EMPTY;
		std::stack<std::unique_ptr<Node>> step_stack_;
	};

	class ArrayItemContext {
	public:
        ArrayItemContext(const Builder& builder) : b_refer_(const_cast<Builder&>(builder)) {}

		ArrayItemContext Value(const Data& value) {
			return ArrayItemContext(b_refer_.Value(value));
		}
		ArrayItemContext Value(Data&& value) {
			return ArrayItemContext(b_refer_.Value(std::move(value)));
		}

		DictItemContext StartDict();

		ArrayItemContext StartArray() {
			return this->b_refer_.StartArray();
		}

		Builder&& EndArray() {
			return std::move(this->b_refer_.EndArray());
		}
	private:
		Builder& b_refer_;
	};

	class DictItemContext {
	public:
        DictItemContext(const Builder& builder) : b_refer_(const_cast<Builder&>(builder)) {}

		ValueItemContext Key(const std::string& key);

		Builder&& EndDict() {
			return  std::move(this->b_refer_.EndDict());
		}
	private:
		Builder& b_refer_;
	};

	class ValueItemContext {
	public:
        ValueItemContext(const Builder& builder) : b_refer_(const_cast<Builder&>(builder)) {}

		DictItemContext Value(const Data& value) {
			return DictItemContext(this->b_refer_.Value(value));
		}
		DictItemContext Value(Data&& value) {
			return DictItemContext(this->b_refer_.Value(std::move(value)));
		}

		DictItemContext StartDict() {
			return this->b_refer_.StartDict();
		}

		ArrayItemContext StartArray() {
			return this->b_refer_.StartArray();
		}
	private:
		Builder& b_refer_;
	};
} //json
