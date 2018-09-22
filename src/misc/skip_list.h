#pragma once

#include <cassert>
#include <functional> // equal_to
#include <algorithm> // min
#include <random>
#include "error.h"
//#include "log.h"

template<typename KeyType, typename ValueType, int I>
class SkipListNode
{
public:
    explicit SkipListNode(const KeyType &key) : _key(key), _value()
    {
        for (int i = 0; i < I; i++)
        {
            _forward_array[i] = nullptr;
            _span_array[i] = 0;
        }
    }

    ~SkipListNode() = default;

    static SkipListNode * create(const KeyType &key)
    {
        return new SkipListNode(key);
    }

    void purge()
    {
        delete this;
    }

    const KeyType & get_key() const
    {
        return _key;
    }

    void set_key(const KeyType &key)
    {
        _key = key;
    }

    ValueType & get_value()
    {
        return _value;
    }

    const ValueType & get_value() const
    {
        return _value;
    }

    void set_value(const ValueType &value)
    {
        _value = value;
    }

    SkipListNode *& forward(int level)
    {
        assert(level >= 0 && level < I);
        return _forward_array[level];
    }

    unsigned int & span(int level)
    {
        assert(level >= 0 && level < I);
        return _span_array[level];
    }

    void dump(bool header)
    {
        /*
        if (header)
        {
            log_info("(header)");
        }
        else
        {
            log_info("(%d)", (int)get_key());
        }

        if (_span_array[0])
        {
            log_info("\nforward:  ");
            for (int i = 0; i < I; i++)
            {
                if (!span(i))
                    break;

                if (_forward_array[i])
                {
                    log_info("\t%d(%d)", (int)_forward_array[i]->get_key(), span(i));
                }
                else
                {
                    log_info("\tnil(%d)", span(i));
                }
            }
        }
        log_info("\n------------\n");
        */
    }

private:
    KeyType     _key;
    ValueType   _value;

    SkipListNode   *_forward_array[I];
    unsigned int    _span_array[I];
};

template<typename KeyType, typename ValueType, typename CompareType, int I>
class SkipList final
{
public:
    typedef SkipListNode<KeyType, ValueType, I> NodeType;

    explicit SkipList(int rand_max)
        : _header_node(nullptr), _level(-1), _length(0), _compare_policy(), _random_engine(), _random_dis(0, rand_max - 1)
    {
        _header_node = NodeType::create(KeyType());
    }

    ~SkipList()
    {
        NodeType *node = _header_node;
        while (node)
        {
            NodeType *next = node->forward(0);
            node->purge();
            node = next;
        }
    }

    NodeType * first_node()
    {
        return _header_node->forward(0);
    }

    // [key
    NodeType * lower_bound(const KeyType &key)
    {
        NodeType *node = _header_node;
        auto equal_to = std::equal_to<KeyType>();

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                const KeyType &k = next->get_key();
                if (_compare_policy(key, k) || equal_to(key, k))
                    break;

                node = next;
            }
        }

        return node->forward(0);
    }

    // (key
    NodeType * upper_bound(const KeyType &key)
    {
        NodeType *node = _header_node;

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                const KeyType &k = next->get_key();
                if (_compare_policy(key, k))
                    break;

                node = next;
            }
        }

        return node->forward(0);
    }

    unsigned int lower_rank(const KeyType &key)
    {
        unsigned int rank = 0;
        NodeType *node = _header_node;
        auto equal_to = std::equal_to<KeyType>();

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                const KeyType &k = next->get_key();
                if (_compare_policy(key, k) || equal_to(key, k))
                    break;

                rank += node->span(i);
                node = next;
            }
        }

        return rank;
    }

    unsigned int upper_rank(const KeyType &key)
    {
        unsigned int rank = 0;
        NodeType *node = _header_node;

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                const KeyType &k = next->get_key();
                if (_compare_policy(key, k))
                    break;

                rank += node->span(i);
                node = next;
            }
        }

        return rank;
    }

    NodeType * operator [](unsigned int rank)
    {
        if (rank >= _length)
            return nullptr;

        NodeType *node = _header_node;

        for (int i = _level; i >= 0 && rank > 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                unsigned int span = node->span(i);
                if (span > rank)
                    break;

                node = next;
                rank -= span;
            }
        }

        return node->forward(0);
    }

    unsigned int length()
    {
        return _length;
    }

    NodeType * insert(NodeType *target_node, int level = -1)
    {
        unsigned int target_span = target_node->span(0);
        logic_assert(target_span == 0, "span(0) = %d", target_span);

        if (level < 0)
        {
            level = 0;
            while ((_random_dis(_random_engine) == 0) && (level < I - 1))
                ++level;
        }

        const KeyType &key = target_node->get_key();
        NodeType *pre_nodes[I];
        unsigned int spans[I];
        unsigned int rank = 0;
        NodeType *node = _header_node;

        _length++;
        
        if (level > _level)
        {
            for (int i = _level + 1; i <= level; i++)
                _header_node->span(i) = _length;

            _level = level;
        }

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                const KeyType &k = next->get_key();
                if (_compare_policy(key, k))
                    break;

                rank += node->span(i);
                node = next;
            }

            pre_nodes[i] = node;
            spans[i] = rank;
        }

        for (int i = 0; i <= level; i++)
        {
            NodeType *pre_node = pre_nodes[i];
            NodeType *next = pre_node->forward(i);
            unsigned int span = rank - spans[i];

            target_node->forward(i) = next;
            target_node->span(i) = pre_node->span(i) - span;

            pre_node->forward(i) = target_node;
            pre_node->span(i) = span + 1;
        }

        for (int i = level + 1; i <= _level; i++)
        {
            pre_nodes[i]->span(i)++;
        }

        return target_node;
    }

    NodeType * remove(unsigned int rank, unsigned int count)
    {
        if (rank >= _length)
            return nullptr;

        count = std::min(_length - rank, count);
        if (count == 0)
            return nullptr;

        NodeType *pre_nodes[I];
        NodeType *node = _header_node;

        for (int i = _level; i >= 0; i--)
        {
            while (true)
            {
                NodeType *next = node->forward(i);
                if (!next)
                    break;

                unsigned int span = node->span(i);
                if (span > rank)
                    break;

                node = next;
                rank -= span;
            }

            pre_nodes[i] = node;
        }

        NodeType *result = nullptr;
        for (unsigned int j = 0; j < count; j++)
        {
            NodeType *target_node = node->forward(0);
            assert(target_node != nullptr);

            for (int i = 0; i <= _level; i++)
            {
                if (pre_nodes[i]->forward(i) != target_node)
                    break;

                pre_nodes[i]->forward(i) = target_node->forward(i);
                pre_nodes[i]->span(i) += target_node->span(i);

                target_node->forward(i) = nullptr;
                target_node->span(i) = 0;
            }

            target_node->forward(0) = result;
            result = target_node;
        }

        for (int i = 0; i <= _level; i++)
        {
            pre_nodes[i]->span(i) -= count;
        }

        while (_level >= 0 && !_header_node->forward(_level))
        {
            _header_node->span(_level) = 0;
            _level--;
        }

        _length -= count;
        return result;
    }

    void add(const KeyType &key, int level)
    {
        insert(NodeType::create(key), level);
    }

    NodeType * create(const KeyType &key, const ValueType &value)
    {
        NodeType *node = NodeType::create(key);
        node->set_value(value);
        return insert(node);
    }

    void dump()
    {
        log_info("%s\n", "<< SkipList dump Info >> ");
        log_info("Level = %d, Length = %u\n\n", _level, _length);

        _header_node->dump(true);
        for (NodeType *node = first_node(); node; node = node->forward(0))
        {
            node->dump(false);
        }
    }


private:
    NodeType       *_header_node;
    int             _level;
    unsigned int    _length;
    CompareType     _compare_policy;
    std::default_random_engine _random_engine;
    std::uniform_int_distribution<int> _random_dis;
};
