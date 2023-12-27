#include "rfid_positioning.h"
#include "MFRC522.h"
#include <map>
#include "tag_angles.h"
#include <cstdint>

using namespace std;

RfidPositioning::RfidPositioning() {
    reader = new MFRC522();
}

void RfidPositioning::init() {
    reader->PCD_Init();

    int version = reader->PCD_ReadRegister(MFRC522::PCD_Register::VersionReg);
    debug("[rfid] Firmware version: 0x%x\n", version);
}

uint64_t RfidPositioning::get_present_card_uid() {
    unsigned int uid = 0;

    if(reader->PICC_IsNewCardPresent() && reader->PICC_ReadCardSerial()) {
        uid = *((uint64_t *)reader->uid.uidByte);
    }

    return uid;
}

int RfidPositioning::get_present_card_index() {
    unsigned int uid = get_present_card_uid();
    if(uid == 0) {
        return -1;
    }

    auto index_it = tags.find(uid);
    if(index_it == tags.end())
        return -1;

    return index_it->second;
}