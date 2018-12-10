#ifndef HTML_H
#define HTML_H

#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <set>

#if __cplusplus <= 199711L
    #if linux
        #include <tr1/memory>
    #else
        #include <memory>
    #endif
    using std::tr1::enable_shared_from_this;
    using std::tr1::shared_ptr;
    using std::tr1::weak_ptr;
#else
    #include <memory>
    using std::enable_shared_from_this;
    using std::shared_ptr;
    using std::weak_ptr;
#endif

class HtmlElement : public enable_shared_from_this<HtmlElement> {
    public:
        friend class HtmlParser;
        friend class HtmlDocument;
        HtmlElement() {}
        HtmlElement(shared_ptr<HtmlElement> p) : _parent(p) {}
        std::string getAttribute(const std::string &k) {
            if (attributes.find(k) != attributes.end()) {
                return attributes[k];
            }
            return "";
        }
        shared_ptr<HtmlElement> getElementById(const std::string &id) {
            return getElementById(shared_from_this(), id);
        }
        std::vector<shared_ptr<HtmlElement> > getElementByClassName(const std::string &name) {
            std::vector<shared_ptr<HtmlElement> > result;
            HtmlElement::getElementByClassName(shared_from_this(), name, result);
            return result;
        }
        std::vector<shared_ptr<HtmlElement> > getElementByTagName(const std::string &name) {
            std::vector<shared_ptr<HtmlElement> > result;
            HtmlElement::getElementByTagName(shared_from_this(), name, result);
            return result;
        }
        shared_ptr<HtmlElement> parent() {
            return _parent.lock();
        }
        const std::string &text() {
            return value;
        }
        const std::string &tagName() {
            return name;
        }
        shared_ptr<HtmlElement> first_child() {
            return *childrens->begin();
        }
        shared_ptr<HtmlElement> last_child() {
            return *childrens->end();
        }
        shared_ptr<HtmlElement> next() {
            if (_parent) {
                size_t idx = 0;
                size_t count = _parent->childrens.size();
                while (idx < count && this != _parent->childrens[idx]) {
                    ++idx;
                }
                if (++idx >= count) {
                    return shared_ptr<HtmlElement>();
                }
                return _parent->childrens[idx];
            }
            return shared_ptr<HtmlElement>();
        }
        shared_ptr<HtmlElement> prev() {
            if (_parent) {
                size_t idx = 0;
                size_t count = _parent->childrens.size();
                while (idx < count && this != _parent->childrens[idx]) {
                    ++idx;
                }
                if (--idx < 0) {
                    return shared_ptr<HtmlElement>();
                }
                return _parent->childrens[idx];
            }
            return shared_ptr<HtmlElement>();
        }
        shared_ptr<HtmlElement> children(size_t index) {
            if (index < 0) {
                index = childrens.size() + index;
            }
            if (index < 0 || index >= childrens.size()) {
                return shared_ptr<HtmlElement>();
            }
            return childrens[index];
        }
    private:
        static shared_ptr<HtmlElement> getElementById(const shared_ptr<HtmlElement> &element, const std::string &id) {
            std::vector<shared_ptr<HtmlElement> >::const_iterator it;
            for (it = element->childrens.begin(); it != element->childrens.end(); ++it) {
                if ((*it)->getAttribute("id") == id) {
                    return *it;
                }
                shared_ptr<HtmlElement> r = getElementById(*it, id);
                if (r) {
                    return r;
                }
            }
            return shared_ptr<HtmlElement>();
        }
        static void getElementByClassName(const shared_ptr<HtmlElement> &element, const std::string &name, std::vector<shared_ptr<HtmlElement> > &result) {
            std::vector<shared_ptr<HtmlElement> >::const_iterator it;
            for (it = element->childrens.begin(); it != element->childrens.end(); ++it) {
                std::set<std::string> attr_class = SplitClassName((*it)->getAttribute("class"));
                std::set<std::string> class_name = SplitClassName(name);
                std::set<std::string>::const_iterator iter = class_name.begin();
                for(; iter != class_name.end(); ++iter){
                    if(attr_class.find(*iter) == attr_class.end()){
                        break;
                    }
                }
                if(iter == class_name.end()){
                    result.push_back(*it);
                }
                getElementByClassName(*it, name, result);
            }
        }
        static void getElementByTagName(const shared_ptr<HtmlElement> &element, const std::string &name, std::vector<shared_ptr<HtmlElement> > &result) {
            std::vector<shared_ptr<HtmlElement> >::const_iterator it;
            for(it = element->childrens.begin(); it != element->childrens.end(); ++it) {
                if ((*it)->name == name){
                    result.push_back(*it);
                }
                getElementByTagName(*it, name, result);
            }
        }
        void parse(const std::string &attr) {
            size_t index = 0;
            std::string k, v;
            char split = ' ';
            bool quota = false;
            enum ParseAttrState {
                PARSE_ATTR_KEY,
                PARSE_ATTR_VALUE_BEGIN,
                PARSE_ATTR_VALUE_END,
            };
            ParseAttrState state = PARSE_ATTR_KEY;
            while (attr.size() > index) {
                char input = attr.at(index);
                switch (state) {
                    case PARSE_ATTR_KEY: {
                        if (input == '\t' || input == '\r' || input == '\n') {
                        } else if (input == '\'' || input == '"') {
                            std::cerr << "WARN : attribute unexpected " << input << std::endl;
                        } else if (input == ' ') {
                            if (!k.empty()) {
                                attribute[k] = v;
                                k.clear();
                            }
                        } else if (input == '=') {
                            state = PARSE_ATTR_VALUE_BEGIN;
                        } else {
                            k.append(attr.c_str() + index, 1);
                        }
                    } break;
                    case PARSE_ATTR_VALUE_BEGIN:{
                        if (input == '\t' || input == '\r' || input == '\n' || input == ' ') {
                            if (!k.empty()) {
                                attribute[k] = v;
                                k.clear();
                            }
                            state = PARSE_ATTR_KEY;
                        } else if (input == '\'' || input == '"') {
                            split = input;
                            quota = true;
                            state = PARSE_ATTR_VALUE_END;
                        } else {
                            v.append(attr.c_str() + index, 1);
                            quota = false;
                            state = PARSE_ATTR_VALUE_END;
                        }
                    } break;
                    case PARSE_ATTR_VALUE_END: {
                        if((quota && input == split) || (!quota && (input == '\t' || input == '\r' || input == '\n' || input == ' '))) {
                            attribute[k] = v;
                            k.clear();
                            v.clear();
                            state = PARSE_ATTR_KEY;
                        } else {
                            v.append(attr.c_str() + index, 1);
                        }
                    } break;
                }
                index++;
            }
            if(!k.empty()){
                attribute[k] = v;
            }
            //trim
            if (!value.empty()) {
                value.erase(0, value.find_first_not_of(" "));
                value.erase(value.find_last_not_of(" ") + 1);
            }
        }
        static std::set<std::string> SplitClassName(const std::string& name){
            #if defined(WIN32)
                #define strtok_ strtok_s
            #else
                #define strtok_ strtok_r
            #endif
            std::set<std::string> class_names;
            char *temp = NULL;
            char *p = strtok_((char *)name.c_str(), " ", &temp);
            while (p) {
                class_names.insert(p);
                p = strtok_(NULL, " ", &temp);
            }
            return class_names;
        }
        std::string name;
        std::string value;
        std::map<std::string, std::string> attributes;
        weak_ptr<HtmlElement> _parent;
        std::vector<shared_ptr<HtmlElement> > childrens;
};

class HtmlDocument {
    public:
        HtmlDocument(shared_ptr<HtmlElement> &root) : root_(root) {}
        shared_ptr<HtmlElement> getElementById(const std::string &id) {
            return HtmlElement::getElementById(root_, id);
        }
        std::vector<shared_ptr<HtmlElement> > getElementByClassName(const std::string &name) {
            std::vector<shared_ptr<HtmlElement> > result;
            HtmlElement::getElementByClassName(root_, name, result);
            return result;
        }
        std::vector<shared_ptr<HtmlElement> > getElementByTagName(const std::string &name) {
            std::vector<shared_ptr<HtmlElement> > result;
            HtmlElement::getElementByTagName(root_, name, result);
            return result;
        }
    private:
        shared_ptr<HtmlElement> root_;
};

class HtmlParser {
    public:
        HtmlParser() {
            static const std::string token[] = { "br", "hr", "img", "input", "link", "meta", "area", "base", "col", "command", "embed", "keygen", "param", "source", "track", "wbr"};
            self_closing_tags_.insert(token, token + sizeof(token) / sizeof(token[0]));
        }
        shared_ptr<HtmlDocument> parse(const char *data, size_t len) {
            stream_ = data;
            length_ = len;
            size_t index = 0;
            root_.reset(new HtmlElement());
            while (length_ > index) {
                char input = stream_[index];
                if (input == '\r' || input == '\n' || input == '\t' || input == ' ') {
                    index++;
                } else if (input == '<') {
                    index = parseElement(index, root_);
                } else {
                    break;
                }
            }
            return shared_ptr<HtmlDocument>(new HtmlDocument(root_));
        }
        shared_ptr<HtmlDocument> parse(const std::string &data) {
            return parse(data.data(), data.size());
        }
    private:
        size_t parseElement(size_t index, shared_ptr<HtmlElement> &element) {
            while (length_ > index) {
                char input = stream_[index + 1];
                if (input == '!') {
                    if (strncmp(stream_ + index, "<!--", 4) == 0) {
                        return SkipUntil(index + 2, "-->");
                    } else {
                        return SkipUntil(index + 2, '>');
                    }
                } else if (input == '/') {
                    return SkipUntil(index, '>');
                } else if (input == '?') {
                    return SkipUntil(index, "?>");
                }
                shared_ptr<HtmlElement> self(new HtmlElement(element));

                enum ParseElementState {
                    PARSE_ELEMENT_TAG,
                    PARSE_ELEMENT_ATTR,
                    PARSE_ELEMENT_VALUE,
                    PARSE_ELEMENT_TAG_END
                };

                ParseElementState state = PARSE_ELEMENT_TAG;
                index++;
                char split = 0;
                std::string attr;

                while (length_ > index) {
                    switch (state) {
                        case PARSE_ELEMENT_TAG: {
                            char input = stream_[index];
                            if (input == ' ' || input == '\r' || input == '\n' || input == '\t') {
                                if (!self->name.empty()) {
                                    state = PARSE_ELEMENT_ATTR;
                                }
                                index++;
                            } else if (input == '/') {
                                self->parse(attr);
                                element->childrens.push_back(self);
                                return SkipUntil(index, '>');
                            } else if (input == '>') {
                                if(self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                                    element->childrens.push_back(self);
                                    return ++index;
                                }
                                state = PARSE_ELEMENT_VALUE;
                                index++;
                            } else {
                                self->name.append(stream_ + index, 1);
                                index++;
                            }
                        } break;
                        case PARSE_ELEMENT_ATTR: {
                            char input = stream_[index];
                            if (input == '>') {
                                if (stream_[index - 1] == '/') {
                                    attr.erase(attr.size() - 1);
                                    self->parse(attr);
                                    element->childrens.push_back(self);
                                    return ++index;
                                } else if(self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                                    self->parse(attr);
                                    element->childrens.push_back(self);
                                    return ++index;
                                }
                                state = PARSE_ELEMENT_VALUE;
                                index++;
                            } else {
                                attr.append(stream_ + index, 1);
                                index++;
                            }
                        } break;
                        case PARSE_ELEMENT_VALUE: {
                            if (self->name == "script" || self->name == "noscript" || self->name == "style") {
                                std::string close = "</" + self->name + ">";
                                size_t pre = index;
                                index = SkipUntil(index, close.c_str());
                                if (index > (pre + close.size()))
                                    self->value.append(stream_ + pre, index - pre - close.size());
                                self->parse(attr);
                                element->childrens.push_back(self);
                                return index;
                            }
                            char input = stream_[index];
                            if (input == '<') {
                                if (stream_[index + 1] == '/') {
                                    state = PARSE_ELEMENT_TAG_END;
                                } else {
                                    index = parseElement(index, self);
                                }
                            } else if (input != '\r' && input != '\n' && input != '\t') {
                                self->value.append(stream_ + index, 1);
                                index++;
                            } else {
                                index++;
                            }
                        } break;
                        case PARSE_ELEMENT_TAG_END: {
                            index += 2;
                            std::string selfname = self->name + ">";
                            if (strncmp(stream_ + index, selfname.c_str(), selfname.size())) {
                                int pre = index;
                                index = SkipUntil(index, ">");
                                std::string value;
                                if (index > (pre + 1))
                                    value.append(stream_ + pre, index - pre - 1);
                                else
                                    value.append(stream_ + pre, index - pre);
                                shared_ptr<HtmlElement> parent = self->GetParent();
                                while (parent) {
                                    if (parent->name == value) {
                                        std::cerr << "WARN : element not closed <" << self->name << "> " << std::endl;
                                        return pre - 2;
                                    }
                                    parent = parent->parent();
                                }
                                std::cerr << "WARN : unexpected closed element </" << value << "> for <" << self->name
                                          << ">" << std::endl;
                                state = PARSE_ELEMENT_VALUE;
                            } else {
                                self->parse(attr);
                                element->childrens.push_back(self);
                                return SkipUntil(index, '>');
                            }
                        } break;
                    }
                }
            }
            return index;
        }
        size_t SkipUntil(size_t index, const char *data) {
            while (length_ > index) {
                if (strncmp(stream_ + index, data, strlen(data)) == 0) {
                    return index + strlen(data);
                } else {
                    index++;
                }
            }
            return index;
        }

        size_t SkipUntil(size_t index, const char data) {
            while (length_ > index) {
                if (stream_[index] == data) {
                    return ++index;
                } else {
                    index++;
                }
            }
            return index;
        }
        const char *stream_;
        size_t length_;
        std::set<std::string> self_closing_tags_;
        shared_ptr<HtmlElement> root_;
};

#endif
