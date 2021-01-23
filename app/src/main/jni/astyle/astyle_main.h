#ifndef ASTYLE_MAIN_H
#define ASTYLE_MAIN_H

#include "astyle.h"

#include <ctime>
#include <sstream>

#if defined(__BORLANDC__) && __BORLANDC__ < 0x0650

using std::time_t;
#endif

#if defined(_MSC_VER)
#include <sys/stat.h>
#include <sys/utime.h>
#else
#include <sys/stat.h>
#include <utime.h>
#endif

#ifdef ASTYLE_JNI
#include <jni.h>
#ifndef ASTYLE_LIB
#define ASTYLE_LIB
#endif
#endif

#ifndef ASTYLE_LIB

#include "ASLocalizer.h"
#define _(a) localizer.settext(a)
#endif

#if defined(_MSC_VER)
#pragma warning(disable: 4996)
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif

#ifdef ASTYLE_LIB

#ifdef _WIN32
#ifndef STDCALL
#define STDCALL __stdcall
#endif

#ifdef ASTYLE_NO_EXPORT
#define EXPORT
#else
#define EXPORT __declspec(dllexport)
#endif

#else
#define STDCALL
#if __GNUC__ >= 4
#define EXPORT __attribute__ ((visibility ("default")))
#else
#define EXPORT
#endif
#endif

typedef void (STDCALL *fpError)(int errorNumber, const char *errorMessage);
typedef char *(STDCALL *fpAlloc)(unsigned long memoryNeeded);
#endif

namespace astyle {

    template<typename T>
    class ASStreamIterator : public ASSourceIterator {
    public:
        bool checkForEmptyLine;

    public:
        explicit ASStreamIterator(T *in);
        ~ASStreamIterator() override;
        bool getLineEndChange(int lineEndFormat) const;
        int  getStreamLength() const override;
        string nextLine(bool emptyLineWasDeleted) override;
        string peekNextLine() override;
        void peekReset() override;
        void saveLastInputLine();
        streamoff tellg() override;

    private:
        T *inStream;
        string buffer;
        string prevBuffer;
        string outputEOL;
        int eolWindows;
        int eolLinux;
        int eolMacOld;
        streamoff streamLength;
        streamoff peekStart;
        bool prevLineDeleted;

    public:
        bool compareToInputBuffer(const string &nextLine_) const {
            return (nextLine_ == prevBuffer);
        }
        const string &getOutputEOL() const {
            return outputEOL;
        }
        streamoff getPeekStart() const override {
            return peekStart;
        }
        bool hasMoreLines() const override {
            return !inStream->eof();
        }
    };

    class ASEncoding {
    private:
        using utf16 = char16_t;
        using utf8  = unsigned char;
        using ubyte = unsigned char;
        enum { SURROGATE_LEAD_FIRST = 0xD800 };
        enum { SURROGATE_LEAD_LAST = 0xDBFF };
        enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
        enum { SURROGATE_TRAIL_LAST = 0xDFFF };
        enum { SURROGATE_FIRST_VALUE = 0x10000 };
        enum eState { eStart, eSecondOf4Bytes, ePenultimate, eFinal };

    public:
        bool   getBigEndian() const;
        int    swap16bit(int value) const;
        size_t utf16len(const utf16 *utf16In) const;
        size_t utf8LengthFromUtf16(const char *utf16In, size_t inLen, bool isBigEndian) const;
        size_t utf8ToUtf16(char *utf8In, size_t inLen, bool isBigEndian, char *utf16Out) const;
        size_t utf16LengthFromUtf8(const char *utf8In, size_t len) const;
        size_t utf16ToUtf8(char *utf16In, size_t inLen, bool isBigEndian,
                           bool firstBlock, char *utf8Out) const;
    };

    class ASConsole;

    class ASOptions {
    public:
#ifdef ASTYLE_LIB
        ASOptions(ASFormatter &formatterArg);
#else
        ASOptions(ASFormatter &formatterArg, ASConsole &consoleArg);
#endif
        string getOptionErrors() const;
        void importOptions(stringstream &in, vector<string> &optionsVector);
        bool parseOptions(vector<string> &optionsVector, const string &errorInfo);

    private:

        ASFormatter &formatter;
        stringstream optionErrors;
#ifndef ASTYLE_LIB
        ASConsole &console;
#endif

        string getParam(const string &arg, const char *op);
        string getParam(const string &arg, const char *op1, const char *op2);
        bool isOption(const string &arg, const char *op);
        bool isOption(const string &arg, const char *op1, const char *op2);
        void isOptionError(const string &arg, const string &errorInfo);
        bool isParamOption(const string &arg, const char *option);
        bool isParamOption(const string &arg, const char *option1, const char *option2);
        void parseOption(const string &arg, const string &errorInfo);
        bool parseOptionContinued(const string &arg, const string &errorInfo);
    };

#ifndef	ASTYLE_LIB

    class ASConsole {
    private:
        ASFormatter &formatter;
        ASEncoding encode;
        ASLocalizer localizer;
        ostream *errorStream;

        bool isRecursive;
        bool isDryRun;
        bool noBackup;
        bool preserveDate;
        bool isVerbose;
        bool isQuiet;
        bool isFormattedOnly;
        bool ignoreExcludeErrors;
        bool ignoreExcludeErrorsDisplay;
        bool useAscii;

        bool bypassBrowserOpen;
        bool hasWildcard;
        size_t mainDirectoryLength;
        bool filesAreIdentical;
        int  filesFormatted;
        int  filesUnchanged;
        bool lineEndsMixed;
        int  linesOut;

        string outputEOL;
        string prevEOL;
        string astyleExePath;
        string optionFileName;
        string origSuffix;
        string projectOptionFileName;
        string stdPathIn;
        string stdPathOut;
        string targetDirectory;
        string targetFilename;

        vector<string> excludeVector;
        vector<bool>   excludeHitsVector;
        vector<string> fileNameVector;
        vector<string> optionsVector;
        vector<string> projectOptionsVector;
        vector<string> fileOptionsVector;
        vector<string> fileName;

    public:
        explicit ASConsole(ASFormatter &formatterArg);
        ASConsole(const ASConsole &)            = delete;
        ASConsole &operator=(ASConsole const &) = delete;
        void convertLineEnds(ostringstream &out, int lineEnd);
        FileEncoding detectEncoding(const char *data, size_t dataSize) const;
        void error() const;
        void error(const char *why, const char *what) const;
        void formatCinToCout();
        vector<string> getArgvOptions(int argc, char **argv);
        bool fileExists(const char *file) const;
        bool fileNameVectorIsEmpty() const;
        ostream *getErrorStream() const;
        bool getFilesAreIdentical() const;
        int  getFilesFormatted() const;
        bool getIgnoreExcludeErrors() const;
        bool getIgnoreExcludeErrorsDisplay() const;
        bool getIsDryRun() const;
        bool getIsFormattedOnly() const;
        bool getIsQuiet() const;
        bool getIsRecursive() const;
        bool getIsVerbose() const;
        bool getLineEndsMixed() const;
        bool getNoBackup() const;
        bool getPreserveDate() const;
        string getLanguageID() const;
        string getNumberFormat(int num, size_t lcid = 0) const;
        string getNumberFormat(int num, const char *groupingArg, const char *separator) const;
        string getOptionFileName() const;
        string getOrigSuffix() const;
        string getProjectOptionFileName() const;
        string getStdPathIn() const;
        string getStdPathOut() const;
        void getTargetFilenames(string &targetFilename_, vector<string> &targetFilenameVector) const;
        void processFiles();
        void processOptions(const vector<string> &argvOptions);
        void setBypassBrowserOpen(bool state);
        void setErrorStream(ostream *errStreamPtr);
        void setIgnoreExcludeErrors(bool state);
        void setIgnoreExcludeErrorsAndDisplay(bool state);
        void setIsDryRun(bool state);
        void setIsFormattedOnly(bool state);
        void setIsQuiet(bool state);
        void setIsRecursive(bool state);
        void setIsVerbose(bool state);
        void setNoBackup(bool state);
        void setOptionFileName(const string &name);
        void setOrigSuffix(const string &suffix);
        void setPreserveDate(bool state);
        void setProjectOptionFileName(const string &optfilepath);
        void setStdPathIn(const string &path);
        void setStdPathOut(const string &path);
        void standardizePath(string &path, bool removeBeginningSeparator = false) const;
        bool stringEndsWith(const string &str, const string &suffix) const;
        void updateExcludeVector(const string &suffixParam);
        vector<string> getExcludeVector() const;
        vector<bool>   getExcludeHitsVector() const;
        vector<string> getFileNameVector() const;
        vector<string> getOptionsVector() const;
        vector<string> getProjectOptionsVector() const;
        vector<string> getFileOptionsVector() const;
        vector<string> getFileName() const;

    private:
        void correctMixedLineEnds(ostringstream &out);
        void formatFile(const string &fileName_);
        string getParentDirectory(const string &absPath) const;
        string findProjectOptionFilePath(const string &fileName_) const;
        string getCurrentDirectory(const string &fileName_) const;
        void getFileNames(const string &directory, const vector<string> &wildcards);
        void getFilePaths(const string &filePath);
        string getFullPathName(const string &relativePath) const;
        string getHtmlInstallPrefix() const;
        string getParam(const string &arg, const char *op);
        bool isHomeOrInvalidAbsPath(const string &absPath) const;
        void initializeOutputEOL(LineEndFormat lineEndFormat);
        bool isOption(const string &arg, const char *op);
        bool isOption(const string &arg, const char *a, const char *b);
        bool isParamOption(const string &arg, const char *option);
        bool isPathExclued(const string &subPath);
        void launchDefaultBrowser(const char *filePathIn = nullptr) const;
        void printHelp() const;
        void printMsg(const char *msg, const string &data) const;
        void printSeparatingLine() const;
        void printVerboseHeader() const;
        void printVerboseStats(clock_t startTime) const;
        FileEncoding readFile(const string &fileName_, stringstream &in) const;
        void removeFile(const char *fileName_, const char *errMsg) const;
        void renameFile(const char *oldFileName, const char *newFileName, const char *errMsg) const;
        void setOutputEOL(LineEndFormat lineEndFormat, const string &currentEOL);
        void sleep(int seconds) const;
        int  waitForRemove(const char *newFileName) const;
        int  wildcmp(const char *wild, const char *data) const;
        void writeFile(const string &fileName_, FileEncoding encoding, ostringstream &out) const;
#ifdef _WIN32
        void displayLastError();
#endif
    };
#else

    class ASLibrary {
    public:
        ASLibrary()          = default;
        virtual ~ASLibrary() = default;

        char16_t *formatUtf16(const char16_t *, const char16_t *, fpError, fpAlloc) const;
        virtual char16_t *convertUtf8ToUtf16(const char *utf8In, fpAlloc fpMemoryAlloc) const;
        virtual char *convertUtf16ToUtf8(const char16_t *utf16In) const;

    private:
        static char *STDCALL tempMemoryAllocation(unsigned long memoryNeeded);

    private:
        ASEncoding encode;
    };

#endif

}

void  STDCALL javaErrorHandler(int errorNumber, const char *errorMessage);
char *STDCALL javaMemoryAlloc(unsigned long memoryNeeded);

extern "C" EXPORT
jstring STDCALL Java_com_zengge_nbmanager_Features_AStyleGetVersion(JNIEnv *env, jclass);
extern "C" EXPORT
jstring STDCALL Java_com_zengge_nbmanager_Features_AStyleMain(JNIEnv *env,
        jobject obj,
        jstring textInJava,
        jstring optionsJava);

#ifdef ASTYLE_LIB
extern "C" EXPORT
char16_t *STDCALL AStyleMainUtf16(const char16_t *pSourceIn,
                                  const char16_t *pOptions,
                                  fpError fpErrorHandler,
                                  fpAlloc fpMemoryAlloc);
#endif

#ifdef ASTYLE_LIB
extern "C" EXPORT char *STDCALL AStyleMain(const char *pSourceIn,
        const char *pOptions,
        fpError fpErrorHandler,
        fpAlloc fpMemoryAlloc);
extern "C" EXPORT const char *STDCALL AStyleGetVersion(void);
#endif

#endif
