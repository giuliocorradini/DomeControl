#pragma once

#include <cstdint>
#include "MFRC522.h"

class RfidPositioning {
    public:
        RfidPositioning();

        /**
         * Initialize the underlying RFID reader and activates the antenna.
         * Call this before using any other function.
        */
        void init();

        /**
         * Get the UID of a card (if present). The UID can be up to 10 bytes long. Only the first 4 are
         * returned for our purposes.
         * 
         * @return the UID of a card is present, 0 otherwise.
        */
        uint64_t get_present_card_uid();

        /**
         * Returns the index of a card, if present. The index corresponds to a given angle according to the
         * dictionary defined in <tag_angles.h>
         * 
         * @return the index of a card. -1 if no card is present.
        */
        int get_present_card_index();

    private:
        MFRC522 *reader;
};
