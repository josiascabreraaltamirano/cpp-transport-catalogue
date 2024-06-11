#include "json.h"

#include <string>
#include <vector>
#include <deque>

namespace json {

    class Builder {
        class BuilderContext;
        class DictBuilderContext; 
        class DictValueContext;
        class ArrayValueContext;
    public:
        Builder() = default;
        DictValueContext Key(std::string key);
        BuilderContext Value(Node::Value value);
        ArrayValueContext StartArray();
        BuilderContext EndArray();
        DictBuilderContext StartDict();
        BuilderContext EndDict();
        const Node& Build();
    private:
        void CheckValueOrder();
        Node MakeObjectElem();

        class BuilderContext {
        public:
            BuilderContext(Builder& builder) 
            : builder_(builder)
            {
            }

            DictValueContext Key(std::string key) {
                return builder_.Key(std::move(key));
            }

            BuilderContext Value(Node::Value value) {
                return builder_.Value(std::move(value));
            }

            ArrayValueContext StartArray() {
                return builder_.StartArray();
            }

            BuilderContext EndArray() {
                return builder_.EndArray();
            }

            DictBuilderContext StartDict() {
                return builder_.StartDict();
            }

            BuilderContext EndDict() {
                return builder_.EndDict();
            }

            const Node& Build() {
                return builder_.Build();
            }

        private:
            Builder& builder_;
        };

        class DictBuilderContext : public BuilderContext {
        public:
            DictBuilderContext(BuilderContext base)
            : BuilderContext(base)
            {
            }

            BuilderContext Value(Node::Value value) = delete;
            ArrayValueContext StartArray() = delete;
            DictBuilderContext StartDict() = delete;
            BuilderContext EndArray() = delete;
            const Node& Build() = delete;
        };

        class DictValueContext : public BuilderContext {
        public:
            DictValueContext(BuilderContext base)
            : BuilderContext(base)
            {
            }

            DictBuilderContext Value(Node::Value value) {
                return BuilderContext::Value(std::move(value));
            }
            DictValueContext Key(std::string key) = delete;
            BuilderContext EndArray() = delete;
            BuilderContext EndDict() = delete;
            const Node& Build() = delete;
        };

        class ArrayValueContext : public BuilderContext {
        public:
            ArrayValueContext(BuilderContext base)
            : BuilderContext(base)
            {
            }

            ArrayValueContext Value(Node::Value value) {
                return BuilderContext::Value(std::move(value));
            }
            
            DictValueContext Key(std::string key) = delete;
            BuilderContext EndDict() = delete;
            const Node& Build() = delete;
        };

        bool is_value_expected = false;
        Node root_;
        std::vector<std::string> keys_buffer_;
        std::deque<Node> nodes_;
        std::vector<Node*> nodes_stack_;
    };

    
} // namespace json
