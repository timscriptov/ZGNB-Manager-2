#ifndef ASLOCALIZER_H
#define ASLOCALIZER_H

#include <string>
#include <vector>

#ifdef ASTYLE_JNI
#ifndef ASTYLE_LIB
#define ASTYLE_LIB
#endif
#endif

namespace astyle {

    using namespace std;

#ifndef ASTYLE_LIB

    class Translation;

    class ASLocalizer {
    public:
        ASLocalizer();
        virtual ~ASLocalizer();
        string getLanguageID() const;
        const Translation *getTranslationClass() const;
#ifdef _WIN32
        void setLanguageFromLCID(size_t lcid);
#endif
        void setLanguageFromName(const char *langID);
        const char *settext(const char *textIn) const;

    private:
        void setTranslationClass();

    private:
        Translation *m_translationClass;
        string m_langID;
        string m_subLangID;
#ifdef _WIN32
        size_t m_lcid;
        size_t m_codepage;
#endif
    };

    class Translation

    {
    public:
        Translation();
        virtual ~Translation() = default;
        string convertToMultiByte(const wstring &wideStr) const;
        string getTranslationString(size_t i) const;
        size_t getTranslationVectorSize() const;
        bool getWideTranslation(const string &stringIn, wstring &wideOut) const;
        string &translate(const string &stringIn) const;

    protected:
        void addPair(const string &english, const wstring &translated);

        vector<pair<string, wstring> > m_translationVector;

    private:

        static const size_t translationElements = 30;

        mutable string m_mbTranslation;
    };

    class Bulgarian : public Translation {
    public:
        Bulgarian();
    };

    class ChineseSimplified : public Translation {
    public:
        ChineseSimplified();
    };

    class ChineseTraditional : public Translation {
    public:
        ChineseTraditional();
    };

    class Dutch : public Translation {
    public:
        Dutch();
    };

    class English : public Translation {
    public:
        English();
    };

    class Estonian : public Translation {
    public:
        Estonian();
    };

    class Finnish : public Translation {
    public:
        Finnish();
    };

    class French : public Translation {
    public:
        French();
    };

    class German : public Translation {
    public:
        German();
    };

    class Greek : public Translation {
    public:
        Greek();
    };

    class Hindi : public Translation {
    public:
        Hindi();
    };

    class Hungarian : public Translation {
    public:
        Hungarian();
    };

    class Italian : public Translation {
    public:
        Italian();
    };

    class Japanese : public Translation {
    public:
        Japanese();
    };

    class Korean : public Translation {
    public:
        Korean();
    };

    class Norwegian : public Translation {
    public:
        Norwegian();
    };

    class Polish : public Translation {
    public:
        Polish();
    };

    class Portuguese : public Translation {
    public:
        Portuguese();
    };

    class Romanian : public Translation {
    public:
        Romanian();
    };

    class Russian : public Translation {
    public:
        Russian();
    };

    class Spanish : public Translation {
    public:
        Spanish();
    };

    class Swedish : public Translation {
    public:
        Swedish();
    };

    class Ukrainian : public Translation {
    public:
        Ukrainian();
    };

#endif

}

#endif
