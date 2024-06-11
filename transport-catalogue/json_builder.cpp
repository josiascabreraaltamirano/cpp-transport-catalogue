#include "json_builder.h"

#include <utility>
#include <exception>
#include <string>
#include <iostream>
#include <algorithm>

namespace json {
    using namespace std::literals;
    Builder::DictValueContext Builder::Key(std::string key) {
        if (nodes_stack_.empty() || !nodes_stack_.back() -> IsDict()) {
            throw std::logic_error("Attempt to insert a key in a non-existent Dict"s);
        }

        if (is_value_expected) {
            throw std::logic_error("Attempt to insert a key when a value is expected"s);
        }

        keys_buffer_.push_back(std::move(key));
        is_value_expected = true;

        return BuilderContext{*this};
    }

    Builder::BuilderContext Builder::Value(Node::Value value) {
        CheckValueOrder();

        nodes_.emplace_back(value);
        return BuilderContext{*this};
    }

    Builder::ArrayValueContext Builder::StartArray() {
        CheckValueOrder();

        nodes_.emplace_back(Array{});
        nodes_stack_.push_back(&nodes_.back());
        return BuilderContext{*this};
    }

    Builder::BuilderContext Builder::EndArray() {
        if (nodes_.empty() || nodes_stack_.empty()) {
            throw std::logic_error("Attempt to close a non-existent Array"s);
        }
        
        auto& to_build = nodes_stack_.back();

        if (!to_build -> IsArray()) {
            throw std::logic_error("Attempt to close an Array, when the current object is a Dict"s);
        }

        Array buffer;
        while (&nodes_.back() != to_build) {
            buffer.push_back(MakeObjectElem());
        }

        std::reverse(buffer.begin(), buffer.end());

        to_build -> GetValue() = std::move(buffer);
        nodes_stack_.pop_back();
        return BuilderContext{*this};
    }

    Builder::DictBuilderContext Builder::StartDict() {
        CheckValueOrder();

        nodes_.emplace_back(Dict{});
        nodes_stack_.push_back(&nodes_.back());
        return BuilderContext{*this};
    }

    Builder::BuilderContext Builder::EndDict() {
        if (nodes_.empty() || nodes_stack_.empty()) {
            throw std::logic_error("Attempt to close a non-existent Dict"s);
        }

        if (is_value_expected) {
            throw std::logic_error("Impossible to build a Dict using only keys"s);
        }

        auto& to_build = nodes_stack_.back();

        if (!to_build -> IsDict()) {
            throw std::logic_error("Attempt to close an Dict, when the current object is an Array"s);
        }

        Dict buffer;
        while (&nodes_.back() != to_build) {
            buffer[std::move(keys_buffer_.back())] = MakeObjectElem();
            keys_buffer_.pop_back();
        }

        to_build -> GetValue() = std::move(buffer);
        nodes_stack_.pop_back();
        
        return BuilderContext{*this};
    }
    
    const Node& Builder::Build() {
        using namespace std::literals;
        if (nodes_.empty()) {
            throw std::logic_error("Impossible to build an empty object"s);
        }

        if (!nodes_stack_.empty()) {
            throw std::logic_error("An object typed Array or Dict was started but was not closed"s);
        }
        
        root_.GetValue() = std::move(nodes_.back().GetValue());
        //delete the node that was just assigned
        nodes_.pop_back();
        if (!nodes_.empty()) {
            throw std::logic_error("Impossible to asign more than one value to the same root"s);
        }

        return root_;
    }

    //private function class members
    void Builder::CheckValueOrder() {
        if (!nodes_stack_.empty() && nodes_stack_.back() -> IsDict() && !is_value_expected) {
            throw std::logic_error("Attempt to insert a value in a Dict without a matching key"s);
        }
        is_value_expected = false;
    }

    Node Builder::MakeObjectElem() {
        Node value = std::move(nodes_.back());
        nodes_.pop_back();
        return value;
    }
    //class members

}


        