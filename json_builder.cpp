#include "json_builder.h"

namespace json {
    bool Builder::IsDictKeyTop() {
        return (this->step_stack_.size() > 1) && this->step_stack_.top()->IsString();
    }

    void Builder::Clean_all() {
        while (!this->step_stack_.empty()) {
            this->step_stack_.pop();
        }
        this->state_ = state::EMPTY;
    }

    Node&& Builder::Build() {
        if (this->state_ == state::READY) {
            return std::move(*this->step_stack_.top());
        } else {
            throw std::logic_error("building of unready node");
        }
    }

    ValueItemContext Builder::Key(const std::string& key) {
        switch (this->state_) {
        case state::EMPTY: throw std::logic_error("empty node key add attempt"); break;
        case state::EDITION: {
            if (this->step_stack_.top().get()->IsMap()) {
                this->step_stack_.push((std::make_unique<Node>(key)));
            } else {
                throw std::logic_error(IsDictKeyTop() ? "dict key entered twice" : "not dict node key add attempt");
            }
        } break;
        case state::READY: throw std::logic_error("ready node key add attempt"); break;
        default: throw std::logic_error("dict key common error");
        }

        return ValueItemContext(*this);
    }

    ValueItemContext Builder::Key(std::string&& key) {
        switch (this->state_) {
        case state::EMPTY: throw std::logic_error("empty node key add attempt"); break;
        case state::EDITION: {
            if (this->step_stack_.top().get()->IsMap()) {
                this->step_stack_.push(std::make_unique<Node>(std::move(key)));
            } else {
                throw std::logic_error(IsDictKeyTop() ? "dict key entered twice" : "not dict node key add attempt");
            }
        } break;
        case state::READY: throw std::logic_error("ready node key add attempt"); break;
        default: throw std::logic_error("dict key common error");
        }

        return ValueItemContext(*this);
    }

    DictItemContext Builder::StartDict() {
        switch (this->state_) {
        case state::EMPTY: this->state_ = state::EDITION; this->step_stack_.push(std::make_unique<Node>(Dict())); break;
        case state::EDITION:
            if (!this->step_stack_.top().get()->IsMap()) {
                this->step_stack_.push(std::move(std::make_unique<Node>(Dict())));
            } else {
                throw std::logic_error("start dict in another dict error");
            } break;
        case state::READY: throw std::logic_error("ready node start dict attempt"); break;
        default: throw std::logic_error("start dict common error");
        }
        return DictItemContext(*this);
    }

    ArrayItemContext Builder::StartArray() {
        switch (this->state_) {
        case state::EMPTY: this->state_ = state::EDITION; this->step_stack_.push(std::make_unique<Node>(Array())); break;
        case state::EDITION: {
            if (this->step_stack_.top().get()->IsMap()) {
                throw std::logic_error("start array error. enter a dict key first");
            }
            this->step_stack_.push(std::move(std::make_unique<Node>(Array())));
        } break;
        case state::READY: throw std::logic_error("ready node start array attempt"); break;
        default: throw std::logic_error("start array error");
        }
        return ArrayItemContext(*this);
    }

    Builder& Builder::Value(const Data& value) {
        switch (this->state_) {
        case state::EMPTY: {
            this->step_stack_.push(std::move(std::make_unique<Node>(std::visit([](auto& value) {return Node(value); }, value))));
            this->state_ = state::READY;
        } break;
        case state::EDITION: {
            if (this->step_stack_.top()->IsArray()) {
                json::Array tmp = this->step_stack_.top()->AsArray();
                tmp.emplace_back(std::visit([](auto& value) {return Node(value); }, value));
                *this->step_stack_.top() = Node(tmp);
            } else if (IsDictKeyTop()) {
                std::string key = this->step_stack_.top()->AsString();
                this->step_stack_.pop();
                json::Dict dict = this->step_stack_.top().get()->AsMap();
                dict.insert({ key, std::visit([](auto& value) {return Node(value); }, value) });
                *this->step_stack_.top() = Node(dict);
            } else {
                throw std::logic_error("dict value without key add attempt");
            }
        } break;
        case state::READY: throw std::logic_error("ready node value add attempt"); break;
        default: throw std::logic_error("value common error");
        }
        return *this;
    }

    Builder& Builder::Value(Data&& value) {
        switch (this->state_) {
        case state::EMPTY: {
            this->step_stack_.push(std::move(std::make_unique<Node>(std::visit([](auto& value) {return Node(value); }, value))));
            this->state_ = state::READY;
        } break;
        case state::EDITION: {
            if (this->step_stack_.top()->IsArray()) {
                json::Array tmp = this->step_stack_.top()->AsArray();
                tmp.emplace_back(std::visit([](auto& value) {return Node(std::move(value)); }, value));
                *this->step_stack_.top() = Node(std::move(tmp));
            } else if (IsDictKeyTop()) {
                std::string key = this->step_stack_.top()->AsString();
                this->step_stack_.pop();
                json::Dict dict = this->step_stack_.top().get()->AsMap();
                dict.insert({ key, std::visit([](auto& value) {return Node(std::move(value)); }, value) });
                *this->step_stack_.top() = Node(std::move(dict));
            } else {
                throw std::logic_error("dict value without key add attempt");
            }
        } break;
        case state::READY: throw std::logic_error("ready node value add attempt"); break;
        default: throw std::logic_error("value common error");
        }
        return *this;
    }

    Builder& Builder::EndDict() {
        switch (this->state_) {
        case state::EMPTY: throw std::logic_error("empty node end dict attempt"); break;
        case state::EDITION: {
            if (this->step_stack_.top().get()->IsMap()) {
                if (this->step_stack_.size() == 1) {
                    this->state_ = state::READY;
                } else {
                    json::Dict value = this->step_stack_.top().get()->AsMap();
                    this->step_stack_.pop();
                    Value(std::move(value));
                }
            } else {
                throw std::logic_error(this->step_stack_.top()->IsString() ? "dict value expected" : "it is not a dict");
            }
        } break;
        case state::READY: throw std::logic_error("ready node end dict attempt"); break;
        default: throw std::logic_error("end dict common error");
        }
        return *this;
    }

    Builder& Builder::EndArray() {
        switch (this->state_) {
        case state::EMPTY: throw std::logic_error("empty node end array attempt"); break;
        case state::EDITION: {
            if (this->step_stack_.top()->IsArray()) {
                if (this->step_stack_.size() == 1) {
                    this->state_ = state::READY;
                } else {
                    json::Array value = this->step_stack_.top()->AsArray();
                    this->step_stack_.pop();
                    Value(std::move(value));
                }
            } else {
                throw std::logic_error("non-array node end array attempt");
            }
        } break;
        case state::READY: throw std::logic_error("ready node end array attempt"); break;
        default: throw std::logic_error("end aray common error");
        }
        return *this;
    }

    DictItemContext ArrayItemContext::StartDict() {
        return this->b_refer_.StartDict();
    }

    ValueItemContext DictItemContext::Key(const std::string& key) {
        return this->b_refer_.Key(key);
    }

} //json
