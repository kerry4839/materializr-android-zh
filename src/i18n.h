#pragma once
#include <cstdio>
#include <string>
#include <vector>

// Interface translation.
//
// The ENGLISH STRING IS THE KEY. tr("Extrude") looks "Extrude" up in the active
// catalogue and returns the translation, or "Extrude" itself when there isn't
// one. Three consequences, all deliberate:
//
//   * wrapping a call site can never change what the app shows until a
//     translation for that exact string exists, so the ~800-site conversion is
//     mechanical and safe to do in tranches;
//   * a missing or half-finished catalogue degrades to English rather than to
//     "ui.dialog.extrude.title" or an empty label;
//   * the source reads as prose, so the code stays greppable by what the user
//     actually sees.
//
// The cost is that editing an English string orphans its translation. That is
// the right trade at this size: an orphan shows English, which is exactly the
// pre-translation behaviour, and the catalogue is checked for orphans by
// tools/i18n_check.py rather than by hoping.
//
// ImGui note: labels of the form "Visible##id" carry a widget IDENTITY after
// the ##. tr() translates ONLY the visible half and re-attaches the id
// untouched, so translating can never break widget identity, focus, or the
// .ini layout. Never hand tr() a bare "##id" - there is nothing to translate.

namespace materializr {

// Keep in sync with Settings::language and tools/i18n_catalogue.py's LANGS.
// English is always 0 and is the fallback for every missing string.
enum class Lang { English = 0, Spanish, Portuguese, French, German, Italian, Chinese, COUNT };

// Active language. Switching is LIVE: ImGui rebuilds every string from scratch
// every frame, and ImGui 1.92 rasterises glyphs on demand, so a language change
// needs no font or atlas work at all.
void setLanguage(Lang l);
Lang language();

// Number of shipped languages, and their names IN THEIR OWN LANGUAGE (a user
// who cannot read the current UI language still has to find their own).
int  languageCount();
const char* languageNativeName(int idx);   // "English", "Espanol", "Francais"
const char* languageEnglishName(int idx);  // "English", "Spanish", "French"


// Translate. Returns `s` unchanged when the active language is English or the
// string has no translation. The returned pointer is valid for the process
// lifetime (catalogue entries are static; "label##id" results are interned).
const char* tr(const char* s);

// Convenience for the printf-style call sites: ImGui::Text(trf("%d faces"), n).
// Identical to tr(); named separately only to make format strings obvious at
// the call site.
inline const char* trf(const char* s) { return tr(s); }

// trList: a `const char*[]` of translated items, for the widgets that take one.
// ImGui::Combo declares its items as `const char* const items[]`, so a plain
// `const char* items[] = {"Round corners", "Sharp corners"}` cannot be
// translated at the point of use - the array IS the translation.
//
// Returns vector<const char*> rather than vector<string> for that exact
// reason: Combo will not take a string*. The pointers are safe to hand out
// because tr() returns storage valid for the process lifetime (catalogue
// entries are static; "label##id" results are interned).
//
// Rebuilt on every call rather than cached in a function-local static: a static
// would be built on the first frame and then keep serving the language that was
// active at the time, so switching language mid-session would leave every combo
// in the old one. These vectors are a handful of short pointers and ImGui
// allocates far more per frame, so rebuilding is not worth optimising away.
//
// Variadic rather than a comma-separated argument because items legitimately
// contain commas ("OpenAI-compatible (OpenAI/Ollama/LM Studio)").
inline std::vector<const char*> trList(const char* const* items, int count) {
    std::vector<const char*> out;
    out.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) out.push_back(tr(items[i]));
    return out;
}
template <typename... Rest>
std::vector<const char*> trList(const char* first, Rest... rest) {
    std::vector<const char*> out;
    out.reserve(1 + sizeof...(rest));
    out.push_back(tr(first));
    const char* tail[] = { rest... };
    for (const char* s : tail) out.push_back(tr(s));
    return out;
}

// trFormat: tr() the format string, then printf it - with exact sizing, so a
// long translated sentence is never cut. It lives here rather than beside the
// length widgets because it is pure stdlib and the MODELLING layer needs it:
// Operation::description() returns a composed sentence ("Duplicate body 3
// offset (1.00, 0.00, 0.00)") that the History panel and the Properties panel
// both show, and fragment-concatenating translated halves would give Chinese
// and German the wrong word order. One key per whole sentence fixes that.
//
// Quantities arrive pre-formatted (fmtLength / fmtVec3 return std::string), so
// catalogue keys carry only %s and %d - never a unit. Every language's
// translation must keep the same specifiers in the same order; tools/
// check_specifiers.py is what enforces it.
namespace detail {
// printf argument adapter: std::string -> c_str(); everything else forwarded.
// The static_assert keeps a stray float-length from reaching a %s.
inline const char* fmtArg(const std::string& s) { return s.c_str(); }
inline const char* fmtArg(const char* s)        { return s; }
inline int                fmtArg(int v)                { return v; }
inline long               fmtArg(long v)               { return v; }
inline long long          fmtArg(long long v)          { return v; }
inline unsigned           fmtArg(unsigned v)           { return v; }
inline unsigned long      fmtArg(unsigned long v)      { return v; }   // size_t on this platform
inline unsigned long long fmtArg(unsigned long long v) { return v; }
inline double             fmtArg(double v)             { return v; }
// A float here is a NON-length numeric printed by its own spec (a %.0f%%
// percentage, a count of turns). Lengths never arrive as raw numbers - they
// come pre-formatted as std::string - so promoting to double is correct.
inline double             fmtArg(float v)              { return v; }
template <class T> void fmtArg(const T&) {
    static_assert(sizeof(T) == 0, "trFormat: pass counts as int, quantities pre-formatted as std::string");
}
} // namespace detail

template <class... A>
inline std::string trFormat(const char* fmt, const A&... a) {
    const char* f = tr(fmt);
    const int n = std::snprintf(nullptr, 0, f, detail::fmtArg(a)...);
    if (n < 0) return f;
    std::string out(static_cast<size_t>(n) + 1, '\0');
    std::snprintf(out.data(), out.size(), f, detail::fmtArg(a)...);
    out.resize(static_cast<size_t>(n));
    return out;
}

// How many catalogue entries the active language has, and how many distinct
// strings tr() has been asked for that had none. Drives the Settings readout
// and tools/i18n_check.py, so "is the translation complete" has an answer that
// is measured rather than assumed.
int translatedCount();
int missingCount();
// Every string tr() was asked for and could not translate, newline separated.
// Empty in English. Used to generate the next translation tranche.
std::string missingReport();

// trDate: a whole date as one string, month name translated, part ORDER chosen
// per language. ImGui::Combo-style item tables can be handled by trList() and a
// single key, but a date cannot: the six languages disagree about the order of
// year / month / day, and the catalogue requires one specifier order for all of
// them (see the comment on trDate in i18n.cpp for why that rule stays). mon0 is
// zero-based, matching struct tm.
std::string trDate(int year, int mon0, int day);

} // namespace materializr
