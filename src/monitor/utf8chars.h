#include <ncursesw/ncurses.h>
#include <unordered_map>
#include <string>

namespace rts2ncurses {

class Utf8Chars {
	private:
		std::unordered_map<std::string, cchar_t> charMap;

	public:
		Utf8Chars() {
			initUtf8();
		}

		void initUtf8() {
			setChar("HLINE", L"\u2500", A_NORMAL);
			setChar("VLINE", L"\u2502", A_NORMAL);
			setChar("ULCORNER", L"\u250c", A_NORMAL);
			setChar("URCORNER", L"\u2510", A_NORMAL);
			setChar("LLCORNER", L"\u2514", A_NORMAL);
			setChar("LRCORNER", L"\u2518", A_NORMAL);
			setChar("BULLET", L"\u2022", A_NORMAL);
			setChar("DIAMOND", L"\u2666", A_NORMAL);
			setChar("BTEE", L"\u2534", A_NORMAL);
			setChar("TTEE", L"\u252c", A_NORMAL);
		}

		void setChar(const std::string& name, const wchar_t* wch, attr_t attr) {
			cchar_t ch;
			setcchar(&ch, wch, attr, 0, NULL);
			charMap[name] = ch;
		}

		const cchar_t& getChar(const std::string& name) {
			return charMap.at(name);
		}
};
}
