#pragma once

#include  <cpp/Logging.hpp>

#include <hardware/flash.h>
#include <pico/stdlib.h>
#include <hardware/sync.h>
#ifdef ENABLE_PICO_MULTICORE
  #include <pico/multicore.h>
#endif

#include <vector>
#include <cstring>
#include <algorithm>

#include <pico/unique_id.h>
#include <pico/platform.h>

extern "C"
{
  #include "../crc64.h"
}

bool operator!=(const pico_unique_board_id_t& id1, const pico_unique_board_id_t& id2)
{
  for (int i=0; i < sizeof(pico_unique_board_id_t::id); ++i)
  {
    if (id1.id[i] != id2.id[i]) return true;
  }
  return false;
}

// FlashStorage lets you write a struct or object to the pico's flash memory for
// persistance. Create a FlashStorage<SavedDataT> object and call readFromFlash()
// to recall the object from flash memory and writeToFlash() to write the object to memory.
// The SavedDataT object is readable and writable at the FlashStorage::data field.
//
// SavedDataT must be standard layout and trivially copyable. It must also be smaller than 4096
// bytes in memory. Simple structs of PODs work best but you can push it a little farther.
//
// If the stored data is shorter than the size of SavedDataT but otherwise valid, a partial
// load is performed, so always sanity check data after load and only add fields to the end 
// for best foward/backward compatibility of saved data with changes to your app.
//
// Persisting pointers or other data will "work" in that the pointers will be saved and restored
// but very likely they will not point to anything valid on read, so don't do that.
//
// Reading from flash on a blank pico will safely fail and leave you with a default-constructed
// SavedDataT object. 
//
// All objects are written twice: to the last and second-to-last flash sectors, making it safe
// to write to flash even when sudden loss of power is possible. If one of the copies is 
// corrupted, this is detected with a CRC check on read and the other copy is used.
//
// Because this class writes to the last 2 flash sectors and this behavior is not customizable,
// obviously don't make more than one of them, or use this class if your program code occupies
// this space.
template <typename SavedDataT>
struct FlashStorage
{
public:
  uint64_t crc;
  size_t size = sizeof(FlashStorage<SavedDataT>);
  pico_unique_board_id_t boardId;

  SavedDataT data;

  FlashStorage()
  {
    // Some static asserts to ensure the template type hasn't broken FlashStorage
    static_assert(FLASH_SECTOR_SIZE >= sizeof(FlashStorage<SavedDataT>), "FlashStorage<T> may not be larger than flash sector size!");
    static_assert(std::is_standard_layout<FlashStorage<SavedDataT>>::value, "FlashStorage<T> must have standard layout.");
    static_assert(std::is_trivially_copyable<FlashStorage<SavedDataT>>::value, "FlashStorage<T> must be trivially copyable.");
  }

  // Write this object to the last two flash sectors. These sectors MUST be free.
  // If program code is in the last two sectors, it will corrupt the pico's firmware
  // and require a reflash.
  // Returns false if flash contents is the same to avoid writing twice
  bool writeToFlash()
  {
    size = sizeof(FlashStorage<SavedDataT>);
    pico_get_unique_board_id(&boardId);
    crc = calculateCrc();
    bool wrote0 = writeToFlashInternal(0);
    bool wrote1 = writeToFlashInternal(1);
    return wrote0 || wrote1;
  }

  // Replace the contents of this object with what is read
  // from flash memory. Returns false if neither flash sector
  // seems to contain a valid object.
  bool readFromFlash()
  {
    for (int i=0; i < 2; ++i)
    {
      FlashStorage<SavedDataT> flashSettings = *flashPtr(i);
      uint64_t crc = flashSettings.calculateCrc();
      if (flashSettings.crc == crc)
      {
        if (flashSettings.clampedSize() != sizeof(FlashStorage<SavedDataT>))
        {
          DEBUG_LOG("Load from flash sector " << i << ": partial");
        }
        else
        {
          DEBUG_LOG("Load from flash sector " << i << ": ok");
        }
        memcpy(this, &flashSettings, flashSettings.clampedSize());
        return true;
      }
      else
      {
        DEBUG_LOG("Load from flash sector " << i << ": failed CRC check");
      }
    }
    return false;
  }

private:
  // Return true if the flash is actually written
  bool __no_inline_not_in_flash_func(writeToFlashInternal)(int sectorOffset)
  {
    // Don't write to the flash if it's already what it needs to be
    if (memcmp(this, flashPtr(sectorOffset), sizeof(FlashStorage<SavedDataT>)) == 0)
    {
      return false;
    }

    // Create a block of memory the size of the flash sector
    std::vector<uint8_t> buffer((sizeof(FlashStorage<SavedDataT>)/FLASH_PAGE_SIZE + (sizeof(FlashStorage<SavedDataT>)%FLASH_PAGE_SIZE > 0 ? 1 : 0)) * FLASH_PAGE_SIZE);
    auto* data = buffer.data();
    auto size = buffer.size();
    memcpy(buffer.data(), this, sizeof(FlashStorage<SavedDataT>));

    // Write to flash!
    #ifdef ENABLE_PICO_MULTICORE
      multicore_lockout_start_blocking();
    #endif
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flashOffsetBytes(sectorOffset), FLASH_SECTOR_SIZE);
    flash_range_program(flashOffsetBytes(sectorOffset), data, size);
    restore_interrupts(ints);
    #ifdef ENABLE_PICO_MULTICORE
      multicore_lockout_end_blocking();
    #endif

    return true; 
  }
  
  size_t clampedSize() const
  {
    return std::clamp(size, sizeof(FlashStorage<uint8_t>), sizeof(FlashStorage<SavedDataT>));
  }
  
  uint64_t calculateCrc() const
  {
    // Get the data pointer to the first data after the CRC value
    // Exclude the CRC from calculating the CRC
    uint8_t* dataPtr = (uint8_t*)this + sizeof(uint64_t);
    uint64_t dataSize = clampedSize() - sizeof(uint64_t);

    return crc64(0, dataPtr, dataSize);
  }

  uint32_t flashOffsetBytes(int sectorOffset)
  {
    return PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE * (sectorOffset + 1);
  }

  const FlashStorage<SavedDataT>* flashPtr(int sectorOffset)
  {
    return (FlashStorage<SavedDataT>*)(XIP_BASE+flashOffsetBytes(sectorOffset));
  }
};

