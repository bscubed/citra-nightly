// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/file_sys/archive_source_sd_savedata.h"
#include "core/file_sys/savedata_archive.h"
#include "core/hle/service/fs/archive.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// FileSys namespace

namespace FileSys {

namespace {

std::string GetSaveDataContainerPath(const std::string& sdmc_directory) {
    return Common::StringFromFormat("%sNintendo 3DS/%s/%s/title/", sdmc_directory.c_str(),
                                    SYSTEM_ID, SDCARD_ID);
}

std::string GetSaveDataPath(const std::string& mount_location, u64 program_id) {
    u32 high = static_cast<u32>(program_id >> 32);
    u32 low = static_cast<u32>(program_id & 0xFFFFFFFF);
    return Common::StringFromFormat("%s%08x/%08x/data/00000001/", mount_location.c_str(), high,
                                    low);
}

std::string GetSaveDataMetadataPath(const std::string& mount_location, u64 program_id) {
    u32 high = static_cast<u32>(program_id >> 32);
    u32 low = static_cast<u32>(program_id & 0xFFFFFFFF);
    return Common::StringFromFormat("%s%08x/%08x/data/00000001.metadata", mount_location.c_str(),
                                    high, low);
}

} // namespace

ArchiveSource_SDSaveData::ArchiveSource_SDSaveData(const std::string& sdmc_directory)
    : mount_point(GetSaveDataContainerPath(sdmc_directory)) {
    LOG_DEBUG(Service_FS, "Directory %s set as SaveData.", mount_point.c_str());
}

ResultVal<std::unique_ptr<ArchiveBackend>> ArchiveSource_SDSaveData::Open(u64 program_id) {
    std::string concrete_mount_point = GetSaveDataPath(mount_point, program_id);
    if (!FileUtil::Exists(concrete_mount_point)) {
        // When a SaveData archive is created for the first time, it is not yet formatted and the
        // save file/directory structure expected by the game has not yet been initialized.
        // Returning the NotFormatted error code will signal the game to provision the SaveData
        // archive with the files and folders that it expects.
        return ResultCode(ErrorDescription::FS_NotFormatted, ErrorModule::FS,
                          ErrorSummary::InvalidState, ErrorLevel::Status);
    }

    auto archive = std::make_unique<SaveDataArchive>(std::move(concrete_mount_point));
    return MakeResult<std::unique_ptr<ArchiveBackend>>(std::move(archive));
}

ResultCode ArchiveSource_SDSaveData::Format(u64 program_id,
                                            const FileSys::ArchiveFormatInfo& format_info) {
    std::string concrete_mount_point = GetSaveDataPath(mount_point, program_id);
    FileUtil::DeleteDirRecursively(concrete_mount_point);
    FileUtil::CreateFullPath(concrete_mount_point);

    // Write the format metadata
    std::string metadata_path = GetSaveDataMetadataPath(mount_point, program_id);
    FileUtil::IOFile file(metadata_path, "wb");

    if (file.IsOpen()) {
        file.WriteBytes(&format_info, sizeof(format_info));
        return RESULT_SUCCESS;
    }
    return RESULT_SUCCESS;
}

ResultVal<ArchiveFormatInfo> ArchiveSource_SDSaveData::GetFormatInfo(u64 program_id) const {
    std::string metadata_path = GetSaveDataMetadataPath(mount_point, program_id);
    FileUtil::IOFile file(metadata_path, "rb");

    if (!file.IsOpen()) {
        LOG_ERROR(Service_FS, "Could not open metadata information for archive");
        // TODO(Subv): Verify error code
        return ResultCode(ErrorDescription::FS_NotFormatted, ErrorModule::FS,
                          ErrorSummary::InvalidState, ErrorLevel::Status);
    }

    ArchiveFormatInfo info = {};
    file.ReadBytes(&info, sizeof(info));
    return MakeResult<ArchiveFormatInfo>(info);
}

std::string ArchiveSource_SDSaveData::GetSaveDataPathFor(const std::string& mount_point,
                                                         u64 program_id) {
    return GetSaveDataPath(GetSaveDataContainerPath(mount_point), program_id);
}

} // namespace FileSys