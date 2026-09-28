#include <csignal>
#include <QCoreApplication>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTextStream>
#include <CLI/CLI.hpp>
#include "commandline.h"
#include "model/pbomodel.h"
#include "exception.h"
#include "model/task/packtask.h"
#include "model/task/unpacktask.h"
#include "settings/getapplicationsettingsmanager.h"
#include "util/log.h"

#ifdef WIN32
#include "argv8bit.h"
#endif

#define LOG(...) LOGGER("Main", __VA_ARGS__)

using namespace std;

namespace pboman3 {
    namespace {
        volatile std::sig_atomic_t interrupted = 0;

        void HandleInterrupt(int) {
            interrupted = 1;
        }

        class ScopedInterruptHandler {
        public:
            ScopedInterruptHandler()
                : previous_(std::signal(SIGINT, HandleInterrupt)) {
                interrupted = 0;
            }

            ~ScopedInterruptHandler() {
                std::signal(SIGINT, previous_);
            }

        private:
            using Handler = void (*)(int);
            Handler previous_;
        };

        [[nodiscard]] QString OutputDirectoryFor(const QString& input, const QString& outputDir,
                                                 const bool besideInput) {
            return besideInput ? QFileInfo(input).absolutePath() : outputDir;
        }

        void PrintTaskResult(const model::task::TaskResult& result) {
            QTextStream err(stderr);
            for (const QString& diagnostic : result.diagnostics)
                err << diagnostic << Qt::endl;
            for (const model::task::TaskRename& rename : result.renames)
                err << "Renamed | " << rename.source << " | " << rename.destination << Qt::endl;
        }

        [[nodiscard]] int ExitCodeFor(const model::task::TaskStatus status) {
            if (status == model::task::TaskStatus::Cancelled)
                return 130;
            return status == model::task::TaskStatus::Failure ? 1 : 0;
        }
    }

    int RunConsolePackOperation(const QStringList& folders, const QString& outputDir,
                                const bool besideInput, const util::ApplicationLogLevel logLevel) {
        util::SetLoggerParameters(logLevel);
        ScopedInterruptHandler interruptHandler;

        const auto settings = settings::GetApplicationSettingsManager()->readSettings();
        model::task::TaskResult batchResult;
        for (const QString& folder : folders) {
            //don't parallelize to avoid mess in the console
            model::task::PackTask task(folder, OutputDirectoryFor(folder, outputDir, besideInput),
                                       settings.packConflictResolutionMode);
            const model::task::TaskResult result = task.execute([] { return interrupted != 0; });
            PrintTaskResult(result);
            batchResult.append(result);
            if (result.status == model::task::TaskStatus::Cancelled)
                break;
        }
        return ExitCodeFor(batchResult.status);
    }

    int RunConsoleUnpackOperation(const QStringList& files, const QString& outputDir, const bool besideInput,
                                  const bool usePboPrefix, const util::ApplicationLogLevel logLevel) {
        util::SetLoggerParameters(logLevel);
        ScopedInterruptHandler interruptHandler;

        const auto settings = settings::GetApplicationSettingsManager()->readSettings();
        model::task::TaskResult batchResult;
        for (const QString& file : files) {
            //don't parallelize to avoid mess in the console
            model::task::UnpackTask task(file, OutputDirectoryFor(file, outputDir, besideInput), usePboPrefix,
                                         settings.unpackConflictResolutionMode);
            const model::task::TaskResult result = task.execute([] { return interrupted != 0; });
            PrintTaskResult(result);
            batchResult.append(result);
            if (result.status == model::task::TaskStatus::Cancelled)
                break;
        }
        return ExitCodeFor(batchResult.status);
    }

    template <CharOrWChar TChr>
    int RunWithCliOptions(int argc, TChr* argv[]) {
        using namespace CLI;
        using namespace pboman3;

        int exitCode;
        App cli;
        const CommandLine cmd(&cli);
        const shared_ptr<CommandLine::Result<TChr>> commandLine = cmd.build<TChr>();
        try {
            cli.parse(argc, argv);
        } catch (const ParseError& ex) {
            const int cliExitCode = cli.exit(ex);
            return cliExitCode == 0 ? 0 : 2;
        }

        if (commandLine->pack.hasBeenSet()) {
            QString outputDir;
            if (commandLine->pack.hasOutputPath())
                outputDir = CommandLine::toQt(commandLine->pack.outputPath);
            else
                outputDir = QDir::currentPath();

            const QStringList folders = CommandLine::toQt(commandLine->pack.folders);
            exitCode = RunConsolePackOperation(folders, outputDir, commandLine->pack.besideInput(),
                                               commandLine->logLevel.get());
        } else if (commandLine->unpack.hasBeenSet()) {
            QString outputDir;
            if (commandLine->unpack.hasOutputPath())
                outputDir = CommandLine::toQt(commandLine->unpack.outputPath);
            else
                outputDir = QDir::currentPath();

            const QStringList files = CommandLine::toQt(commandLine->unpack.files);
            exitCode = RunConsoleUnpackOperation(files, outputDir, commandLine->unpack.besideInput(),
                                                 commandLine->unpack.usePboPrefix(), commandLine->logLevel.get());
        } else {
            //should not normally get here; if did - CLI11 was misconfigured somewhere
            cout << cli.help();
            exitCode = 1;
        }
        return exitCode;
    }

    template <CharOrWChar TChr>
    int RunMain(int argc, TChr* argv[]) {
        QCoreApplication app(argc, argv);
        return RunWithCliOptions(argc, argv);
    }

#ifdef WIN32
    template <>
    int RunMain(int argc, wchar_t* argv[]) {
        QCoreApplication app(argc, Argv8Bit::acquire(argc, argv));
        const auto releaseArgv = qScopeGuard([] { Argv8Bit::release(); });
        return RunWithCliOptions(argc, argv);
    }
#endif
}

void HandleEptr(const std::exception_ptr& ptr) try {
    if (ptr)
        std::rethrow_exception(ptr);
} catch (const std::exception& ex) {
    LOG(critical, "Uncaught exception has been thrown:", ex.what())
}

#ifdef WIN32
int wmain(int argc, wchar_t* argv[]) {
#else
int main(int argc, char* argv[]) {
#endif
    try {
        const int res = pboman3::RunMain(argc, argv);
        return res;
    } catch (const pboman3::AppException& ex) {
        LOG(critical, "Unexpected exception has been thrown:", ex)
        throw;
    } catch (...) {
        LOG(critical, "Unexpected exception has been thrown")
        const auto ex = std::current_exception();
        HandleEptr(ex);
        return 1;
    }
}
