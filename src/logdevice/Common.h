// Copyright (c) 2014, Facebook, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
//
#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "include/Env.h"
#include "logdevice/include/Record.h"
#include "logdevice/include/types.h"
#include "src/util/scoped_file_lock.h"

namespace facebook { namespace logdevice {

inline std::string LogFilename(logid_t logid) {
  return "logdevice_" + std::to_string((int64_t)logid) + ".log";
}

/* Log File Format:

+-Record-+-Field------+-Size--------+
| 1      | LSN        | 8 bytes     |
|        +------------+-------------+
|        | Timestamp  | 8 bytes     |
|        +------------+-------------+
|        | Data size  | 8 bytes     |
|        +------------+-------------+
|        | Data       | "Data size" |
+--------+------------+-------------+
| 2      | ...        | ...         |

*/

/**
 * Structure for the three header fields from the above format diagram.
 */
struct RecordHeader {
  lsn_t lsn;
  uint64_t timestamp;
  uint64_t datasize;
};

/**
 * Utility for reading log files.
 * Calling Next() only reads the header. The data will only be read and
 * allocated on first call to GetData(), otherwise the data is skipped in
 * subsequent call to Next().
 */
struct LogFile {
 public:
  LogFile(logid_t logid, bool waitForLock);
  bool Next();  // Reads next record header, returns false if error/EOF.
  lsn_t GetLSN() const;
  std::chrono::milliseconds GetTimestamp() const;
  Payload GetData();
  uint64_t GetOffset() const { return offset_; }

 private:
  rocketspeed::Env* env_;
  rocketspeed::ScopedFileLock file_lock_;
  std::unique_ptr<rocketspeed::SequentialFile> file_;
  RecordHeader header_;
  std::vector<char> data_;
  bool have_data_ = false;
  uint64_t offset_ = 0;
};

}  // namespace logdevice
}  // namespace facebook