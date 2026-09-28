#pragma once

#include "applicationsettingsmanager.h"
#include <QString>

namespace pboman3::settings {
    class LocalStorageApplicationSettingsManager : public ApplicationSettingsManager {
        Q_OBJECT

    public:
        explicit LocalStorageApplicationSettingsManager(QString iniFile = {});

        static void purge();

        [[nodiscard]] ApplicationSettings readSettings() const override;

        void writeSettings(const ApplicationSettings& settings) override;

    private:
        QString iniFile_;
    };
}
