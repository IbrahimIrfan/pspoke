/* Game-side half of the name-entry bypass.
 *
 * The DS name-entry application (gNamingScreenAppTemplate) is the one screen
 * that kills a real PSP-3001: the port reaches it, constructs it, logs a
 * healthy heap, and then the hardware dies with no [FATAL] line. It renders
 * fine in PPSSPP, so the fault is something the DS screen does that the real
 * GE/firmware refuses, and it is not worth chasing when the PSP has a perfectly
 * good keyboard of its own.
 *
 * So: __wrap_ApplicationManager_New (memprobe.c) swaps the naming template for
 * the one below before the game ever constructs the DS screen. The substitute
 * application runs the firmware OSK from the frame loop (osk.c) and, on exit,
 * fills in exactly the fields NamingScreen_Exit would have filled:
 *
 *   args->nameInputRaw   charcode_t[20], CHAR_EOS terminated
 *   args->textInputStr   the same text as a managed String
 *   args->returnCode     NAMING_SCREEN_CODE_NO_INPUT when nothing was entered
 *
 * Because every naming context in the game - player, rival, Pokemon nickname,
 * PC box, Pal Pad friend, group, Shaymin tablet - is constructed through
 * ApplicationManager_New with this one template, all of them are covered by the
 * single interception point.
 *
 * This file includes only game headers. The PSP side is osk.c.
 */
#include "applications/naming_screen.h"

#include <nitro.h>
#include <string.h>

#include "constants/charcode.h"
#include "constants/heap.h"
#include "constants/narc.h"
#include "constants/string.h"

#include "pch/global_pch.h"

#include "charcode.h"
#include "charcode_util.h"
#include "generated/genders.h"
#include "heap.h"
#include "math_util.h"
#include "message.h"
#include "overlay_manager.h"
#include "string_gf.h"

#include "res/text/bank/generic_names.h"

/* osk.c - plain integer / UTF-16 interface, no game types cross this line. */
extern void PSPNativeOskBegin(const unsigned short *desc, const unsigned short *intext, int limit);
extern int PSPNativeOskFinished(void);
extern int PSPNativeOskAccepted(void);
extern const unsigned short *PSPNativeOskText(void);
extern void PSPNativeOskRelease(void);
extern void PSPNativeMemLog(const char *fmt, ...);

/* Widest raw buffer the game gives us is NamingScreenArgs::nameInputRaw[20]. */
#define OSK_NAME_CAP 19

/* ------------------------------------------------------------- encoding ---
 *
 * The game does not use ASCII or Unicode. It uses its own charcode table
 * (include/constants/charcode.h), a dense enum whose Latin letters and digits
 * happen to be contiguous, so those three ranges are arithmetic. Everything
 * else is spelled out.
 *
 * The table below is deliberately limited to characters the DS naming keyboard
 * can actually produce (the sCharCodesUpper, sCharCodesLower and
 * sCharCodesOthers page tables in naming_screen.c).
 * Anything the OSK yields that the DS keyboard could not have produced is
 * dropped rather than translated, so a name coming out of this path is always
 * a name the original screen could have produced too.
 */
typedef struct CharMapEntry {
    charcode_t cc;
    unsigned short uc;
} CharMapEntry;

static const CharMapEntry sCharMap[] = {
    { CHAR_SPACE, ' ' },
    { CHAR_COMMA, ',' },
    { CHAR_PERIOD, '.' },
    { CHAR_COLON, ':' },
    { CHAR_SEMICOLON, ';' },
    { CHAR_EXCLAMATION, '!' },
    { CHAR_QUESTION, '?' },
    { CHAR_DOUBLE_QUOTE_OPEN, '"' },
    { CHAR_DOUBLE_QUOTE_CLOSE, 0x201D },
    { CHAR_SINGLE_QUOTE_CLOSE, '\'' },
    { CHAR_SINGLE_QUOTE_OPEN, 0x2018 },
    { CHAR_PAREN_OPEN, '(' },
    { CHAR_PAREN_CLOSE, ')' },
    { CHAR_TILDE, '~' },
    { CHAR_AT_SIGN, '@' },
    { CHAR_HASH, '#' },
    { CHAR_PERCENT, '%' },
    { CHAR_PLUS, '+' },
    { CHAR_MINUS, '-' },
    { CHAR_ASTERISK, '*' },
    { CHAR_SLASH, '/' },
    { CHAR_EQUALS, '=' },
    { CHAR_DOT, 0x00B7 },
    { CHAR_ELLIPSIS, 0x2026 },
    { CHAR_MALE, 0x2642 },
    { CHAR_FEMALE, 0x2640 },
};

/* Returns CHAR_EOS for anything the DS keyboard has no key for; callers drop
 * those characters. */
static charcode_t UnicodeToCharCode(unsigned short uc)
{
    u32 i;

    if (uc >= '0' && uc <= '9') {
        return (charcode_t)(CHAR_0 + (uc - '0'));
    }
    if (uc >= 'A' && uc <= 'Z') {
        return (charcode_t)(CHAR_A + (uc - 'A'));
    }
    if (uc >= 'a' && uc <= 'z') {
        return (charcode_t)(CHAR_a + (uc - 'a'));
    }

    for (i = 0; i < NELEMS(sCharMap); i++) {
        if (sCharMap[i].uc == uc) {
            return sCharMap[i].cc;
        }
    }

    return CHAR_EOS;
}

/* Returns 0 for anything with no Unicode equivalent; callers drop those. */
static unsigned short CharCodeToUnicode(charcode_t cc)
{
    u32 i;

    if (cc >= CHAR_0 && cc <= CHAR_9) {
        return (unsigned short)('0' + (cc - CHAR_0));
    }
    if (cc >= CHAR_A && cc <= CHAR_Z) {
        return (unsigned short)('A' + (cc - CHAR_A));
    }
    if (cc >= CHAR_a && cc <= CHAR_z) {
        return (unsigned short)('a' + (cc - CHAR_a));
    }

    for (i = 0; i < NELEMS(sCharMap); i++) {
        if (sCharMap[i].cc == cc) {
            return sCharMap[i].uc;
        }
    }

    return 0;
}

/* ------------------------------------------------------------ prompt text */

static const char *PromptForType(enum NamingScreenType type)
{
    switch (type) {
    case NAMING_SCREEN_TYPE_PLAYER:
        return "Your name?";
    case NAMING_SCREEN_TYPE_RIVAL:
        return "Your rival's name?";
    case NAMING_SCREEN_TYPE_POKEMON:
        return "Nickname?";
    case NAMING_SCREEN_TYPE_BOX:
        return "Box name?";
    case NAMING_SCREEN_TYPE_GROUP:
        return "Group name?";
    case NAMING_SCREEN_TYPE_PAL_PAD:
    case NAMING_SCREEN_TYPE_UNK4:
        return "Friend's name?";
    case NAMING_SCREEN_TYPE_SHAYMIN_TABLET:
    default:
        return "Enter a name.";
    }
}

/* ------------------------------------------------- substitute application */

typedef struct OskNamingApp {
    charcode_t initialRaw[OSK_NAME_CAP + 1];
    int maxChars;
    int heapID;
} OskNamingApp;

/* Captured by the ApplicationManager_New wrapper: the naming screen is never
 * nested (the game runs at most one naming application at a time). */
static int sPendingHeapID = HEAP_ID_APPLICATION;

static BOOL OskNaming_Init(ApplicationManager *appMan, int *state)
{
    NamingScreenArgs *args = (NamingScreenArgs *)ApplicationManager_Args(appMan);
    OskNamingApp *app = ApplicationManager_NewData(appMan, sizeof(OskNamingApp), sPendingHeapID);
    unsigned short desc[64];
    unsigned short intext[OSK_NAME_CAP + 1];
    const char *prompt = PromptForType(args->type);
    int i, n;

    memset(app, 0, sizeof(OskNamingApp));
    app->heapID = sPendingHeapID;
    app->maxChars = args->maxChars;
    if (app->maxChars < 1) {
        app->maxChars = 1;
    }
    if (app->maxChars > OSK_NAME_CAP) {
        app->maxChars = OSK_NAME_CAP;
    }

    for (i = 0; prompt[i] != '\0' && i < 63; i++) {
        desc[i] = (unsigned short)(unsigned char)prompt[i];
    }
    desc[i] = 0;

    /* The DS screen seeds its "unchanged?" comparison buffer from
     * args->textInputStr, so the caller's default (a box's current name, for
     * example) is both the initial text of the keyboard and the value that
     * means "the user changed nothing". Reproduce both. */
    app->initialRaw[0] = CHAR_EOS;
    n = 0;
    if (args->textInputStr != NULL) {
        charcode_t raw[64];
        String_ToChars(args->textInputStr, raw, NELEMS(raw));
        for (i = 0; raw[i] != CHAR_EOS && n < app->maxChars; i++) {
            unsigned short uc = CharCodeToUnicode(raw[i]);
            app->initialRaw[n] = raw[i];
            intext[n] = uc ? uc : ' ';
            n++;
        }
        app->initialRaw[n] = CHAR_EOS;
    }
    intext[n] = 0;

    PSPNativeMemLog("[OSK] naming type=%d maxChars=%d initial=%d chars (heap %d)",
        (int)args->type, app->maxChars, n, app->heapID);

    PSPNativeOskBegin(desc, intext, app->maxChars);
    return TRUE;
}

static BOOL OskNaming_Main(ApplicationManager *appMan, int *state)
{
    (void)appMan;
    (void)state;
    return PSPNativeOskFinished() ? TRUE : FALSE;
}

#define RANDOM_NAME(TYPE) (TYPE##_First + LCRNG_Next() % (TYPE##_Last - TYPE##_First + 1))

/* Byte-for-byte the behaviour of NamingScreen_UseDefaultName, minus the
 * NamingScreen struct: a random generic name for the player and the rival,
 * NAMING_SCREEN_CODE_NO_INPUT for every other context (which tells the caller
 * to keep whatever name it already had). */
static void UseDefaultName(NamingScreenArgs *args, int heapID)
{
    if (args->type == NAMING_SCREEN_TYPE_PLAYER || args->type == NAMING_SCREEN_TYPE_RIVAL) {
        MessageLoader *loader = MessageLoader_Init(
            MSG_LOADER_LOAD_ON_DEMAND,
            NARC_INDEX_MSGDATA__PL_MSG,
            TEXT_BANK_GENERIC_NAMES,
            heapID);
        String *string;
        u32 entry;

        if (args->type == NAMING_SCREEN_TYPE_RIVAL) {
            entry = RANDOM_NAME(GenericNames_Rival);
        } else if (args->playerGenderOrMonSpecies == GENDER_FEMALE) {
            entry = RANDOM_NAME(GenericNames_PlayerFemale);
        } else {
            entry = RANDOM_NAME(GenericNames_PlayerMale);
        }

        string = MessageLoader_GetNewString(loader, entry);
        String_Copy(args->textInputStr, string);
        String_Free(string);
        String_ToChars(args->textInputStr, args->nameInputRaw, 10);
        MessageLoader_Free(loader);

        PSPNativeMemLog("[OSK] no input; generic name entry %d used", (int)entry);
    } else {
        args->returnCode = NAMING_SCREEN_CODE_NO_INPUT;
        PSPNativeMemLog("[OSK] no input; returnCode=NO_INPUT for type %d", (int)args->type);
    }
}

#undef RANDOM_NAME

static BOOL IsAllSpaces(const charcode_t *raw)
{
    int i;

    for (i = 0; raw[i] != CHAR_EOS; i++) {
        if (raw[i] != CHAR_SPACE) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL OskNaming_Exit(ApplicationManager *appMan, int *state)
{
    NamingScreenArgs *args = (NamingScreenArgs *)ApplicationManager_Args(appMan);
    OskNamingApp *app = (OskNamingApp *)ApplicationManager_Data(appMan);
    charcode_t entry[OSK_NAME_CAP + 1];
    int heapID = app->heapID;
    int maxChars = app->maxChars;
    int n = 0;

    (void)state;

    if (PSPNativeOskAccepted()) {
        const unsigned short *text = PSPNativeOskText();
        int i;

        for (i = 0; text[i] != 0 && n < maxChars; i++) {
            charcode_t cc = UnicodeToCharCode(text[i]);
            if (cc != CHAR_EOS) {
                entry[n++] = cc;
            }
        }
    }
    entry[n] = CHAR_EOS;

    /* Same three rejections the DS screen applies in NamingScreen_Exit:
     * nothing typed, identical to the string we started with, or only spaces. */
    if (n == 0
        || CharCode_Compare(entry, app->initialRaw) == 0
        || IsAllSpaces(entry)) {
        UseDefaultName(args, heapID);
    } else {
        CharCode_Copy(args->nameInputRaw, entry);
        String_CopyChars(args->textInputStr, entry);
        PSPNativeMemLog("[OSK] accepted %d chars for naming type %d", n, (int)args->type);
    }

    PSPNativeOskRelease();
    ApplicationManager_FreeData(appMan);
    return TRUE;
}

const ApplicationManagerTemplate gPSPNativeOskNamingTemplate = {
    .init = OskNaming_Init,
    .main = OskNaming_Main,
    .exit = OskNaming_Exit,
    .overlayID = FS_OVERLAY_ID_NONE,
};

/* Called by __wrap_ApplicationManager_New. Returns the template to construct
 * instead of the DS naming screen, and records the heap the caller chose so the
 * substitute allocates out of the same place the real screen's args live. */
const void *PSPNativeNamingOskTemplate(int heapID)
{
    sPendingHeapID = heapID;
    return &gPSPNativeOskNamingTemplate;
}
