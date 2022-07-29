/*
 *  historic.h
 *
 *  C++ container that stores the current and last value of a given variable.
 *  Only binary history is implemented so far.
 */

#pragma once

template<typename T>
class Historic {
    public:
        Historic() {
            current = 0;
            last = 0;
        }

        Historic(T initialVal) {
            current = initialVal;
            last = initialVal;
        }

        operator T() {
            return current;
        }

        Historic<T>& operator=(T& value) {
            last = current;
            current = value;

            return *this;
        }

        T get_last() {
            return last;
        }

        bool has_changed() {
            return current == last;
        }

    private:
        T current;
        T last;
};
