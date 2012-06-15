#ifndef SSERVER_ERASE_ITERATOR_H_INCLUDED
#define SSERVER_ERASE_ITERATOR_H_INCLUDED

#include <boost/iterator/iterator_adaptor.hpp>

template<typename C>
class erase_iterator :
    public boost::iterator_adaptor<erase_iterator<C>,
                                   typename C::iterator,
                                   boost::use_default,
                                   boost::single_pass_traversal_tag> {
    friend class boost::iterator_core_access;
public:
    erase_iterator() : data_(0) {}
    explicit erase_iterator(C& data) :
        erase_iterator::iterator_adaptor_(data.begin()),
        data_(data.empty() ? 0 : &data)
    {}

private:
    void increment() {
        if(data_) {
            this->base_reference() = data_->erase(this->base_reference());
        }
        if(this->base() == data_->end()) data_ = 0;
    }

    bool equal(const erase_iterator& rhs) const {
        return this->base() == rhs.base() || (data_ == 0 && rhs.data_ == 0);
    }

    C* data_;
};

#endif // SSERVER_ERASE_ITERATOR_H_INCLUDED
