#include <gtest/gtest.h>
#include <QTemporaryDir>
#include "../localstorageapplicationsettingsmanager.h"

namespace pboman3::settings::test {
    class LocalStorageApplicationSettingsManagerTest : public ::testing::Test {
    protected:
        QString settingsPath() const {
            return dir.filePath("settings.ini");
        }

        QTemporaryDir dir;
    };

    TEST_F(LocalStorageApplicationSettingsManagerTest, ReadSettings_Reads_Values) {
        LocalStorageApplicationSettingsManager facility(settingsPath());

        constexpr ApplicationSettings savedSettings{
            io::FileConflictResolutionMode::Enum::Copy,
            io::FileConflictResolutionMode::Enum::Overwrite,
            OperationCompleteBehavior::Enum::KeepWindow
        };
        facility.writeSettings(savedSettings);

        const auto readSettings = facility.readSettings();

        ASSERT_EQ(readSettings.packConflictResolutionMode, io::FileConflictResolutionMode::Enum::Copy);
        ASSERT_EQ(readSettings.unpackConflictResolutionMode, io::FileConflictResolutionMode::Enum::Overwrite);
        ASSERT_EQ(readSettings.packUnpackOperationCompleteBehavior, OperationCompleteBehavior::Enum::KeepWindow);
    }

    TEST_F(LocalStorageApplicationSettingsManagerTest, ReadSettings_Reads_Defaults) {
        const LocalStorageApplicationSettingsManager facility(settingsPath());

        const auto readSettings = facility.readSettings();

        ASSERT_EQ(readSettings.packConflictResolutionMode, io::FileConflictResolutionMode::Enum::Abort);
        ASSERT_EQ(readSettings.unpackConflictResolutionMode, io::FileConflictResolutionMode::Enum::Abort);
        ASSERT_EQ(readSettings.packUnpackOperationCompleteBehavior, OperationCompleteBehavior::Enum::KeepWindow);
    }
}
