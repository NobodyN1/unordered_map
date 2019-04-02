#include <stdexcept>
#include <initializer_list>
#include <iterator>
#include <vector>
#include <forward_list>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
public:
    typedef std::pair<const KeyType, ValueType> Element;
    typedef typename std::forward_list<Element>::iterator iterator;
    typedef typename std::forward_list<Element>::const_iterator const_iterator;

private:
    Hash hasher_;
    std::forward_list<Element> elements_;
    // хранит итератор на предыдущий элемент перед нужным
    std::vector<std::forward_list<iterator>> table_;
    size_t size_;

    constexpr static float max_load_factor_ = 1.0;

    // ищет элемент перед нужным по ключу в указанной корзине
    typename std::forward_list<iterator>::iterator
    find_prev_in_bucket(size_t bucket_index, const KeyType& key) {
        auto it = table_[bucket_index].before_begin();
        while (next(it) != table_[bucket_index].end() && !(next(*next(it))->first == key))
            ++it;
        return it;
    }

public:
    HashMap& operator=(const HashMap& other) {
        if (this == &other)
            return *this;
        clear();
        for (auto it = other.begin(); it != other.end(); ++it)
            insert(*it);
        return *this;
    }

    explicit HashMap(Hash h = Hash{}) : size_(0), hasher_(h) {
        table_.resize(1);
    }

    template<typename Iter>
    HashMap(Iter first, Iter last, Hash h = Hash{}) : HashMap(h) {
        for (auto it = first; it != last; ++it)
            insert(*it);
    }

    HashMap(std::initializer_list<Element> init_list, Hash h = Hash{}) : HashMap(h) {
        for (const auto& it : init_list)
            insert(it);
    }

    void rehash() {
        size_t prev_size = table_.size();
        table_.resize(2 * prev_size);
        for (auto it = begin(); it != end(); ++it) {
            size_t h = hasher_(it->first);
            size_t prev_bucket_index = h % prev_size;
            size_t new_bucket_index = h % table_.size();
            if (prev_bucket_index != new_bucket_index) {
                auto jt = find_prev_in_bucket(prev_bucket_index, it->first);
                table_[new_bucket_index].push_front(*next(jt));
                table_[prev_bucket_index].erase_after(jt);
            }
        }
    }

    ValueType& operator[](const KeyType& key) {
        size_t bucket_index = hasher_(key) % table_.size();
        auto it = ++find_prev_in_bucket(bucket_index, key);
        if (it != table_[bucket_index].end())
            return next(*it)->second;
        insert({key, ValueType()});
        return elements_.begin()->second;
    }

    const ValueType& at(const KeyType& key) const {
        size_t bucket_index = hasher_(key) % table_.size();
        auto it = ++find_prev_in_bucket(bucket_index, key);
        if (it == table_[bucket_index].end())
            throw std::out_of_range("");
        return next(*it)->second;
    }

    void clear() {
        for (auto it = elements_.begin(); it != elements_.end(); ++it) {
            size_t bucket_index = hasher_(it->first) % table_.size();
            table_[bucket_index].clear();
        }
        elements_.clear();
        size_ = 0;
    }

    Hash hash_function() const {
        return hasher_;
    }

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return elements_.empty();
    }

    iterator begin() {
        return elements_.begin();
    }

    const_iterator begin() const {
        return elements_.begin();
    }

    iterator end() {
        return elements_.end();
    }

    const_iterator end() const {
        return elements_.end();
    }

    iterator find(const KeyType& key) {
        size_t bucket_index = hasher_(key) % table_.size();
        auto it = ++find_prev_in_bucket(bucket_index, key);
        if (it == table_[bucket_index].end())
            return end();
        return next(*it);
    }

    const_iterator find(const KeyType& key) const {
        size_t bucket_index = hasher_(key) % table_.size();
        auto it = ++find_prev_in_bucket(bucket_index, key);
        if (it == table_[bucket_index].end())
            return end();
        return next(*it);
    }

    void insert(const Element& p) {
        if (find(p.first) != end())
            return;

        // у первого элемента в elements_ должно изменится его значение в table_
        if (size_ > 0) {
            size_t bucket_index = hasher_(begin()->first) % table_.size();
            auto it = ++find_prev_in_bucket(bucket_index, begin()->first);
            elements_.push_front(p);
            *it = begin();
        } else {
            elements_.push_front(p);
        }
        ++size_;

        size_t bucket_index = hasher_(p.first) % table_.size();
        table_[bucket_index].push_front(elements_.before_begin());

        if (static_cast<double>(size_) / table_.size() > max_load_factor_)
            rehash();
    }

    void erase(const KeyType& key) {
        size_t bucket_index = hasher_(key) % table_.size();
        auto it = find_prev_in_bucket(bucket_index, key);

        if (next(it) == table_[bucket_index].end())
            return;

        --size_;
        // итератор на элемент, который надо удалить
        auto jt = next(*next(it));
        if (next(jt) != end()) {      // если он не последний
            size_t tmp_bucket_index = hasher_(next(jt)->first) % table_.size();
            // найдем элемент, следующий после него в elements_, в table_
            auto kt = ++find_prev_in_bucket(tmp_bucket_index, next(jt)->first);
            // назначим элементу после него в elements_ значение в table_ такое же, как и у него
            *kt = *next(it);
        }
        elements_.erase_after(*next(it));
        table_[bucket_index].erase_after(it);
    }
};