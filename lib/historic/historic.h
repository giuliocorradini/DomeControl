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
            changed = true;
        }

        Historic(T initialVal) {
            current = initialVal;
            last = initialVal;
            changed = true;
        }

        operator T() {
            changed = false;
            return current;
        }

        Historic<T>& operator=(T& value) {
            last = current;
            current = value;

            changed = (current != last);

            return *this;
        }

        T get_current() {
            return current;
        }

        T get_last() {
            return last;
        }

        /*
         *  Did value change since last update? (with operator=)
         */
        bool has_changed() {
            return current != last;
        }

        /*
         *  Did value change since last query? (since last call of operator T()?)
         */
        bool is_changed() {
            return changed;
        }

    private:
        T current;
        T last;
        bool changed;
};
