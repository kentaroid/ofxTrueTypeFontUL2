#include "ofxTrueTypeFontUL2.h"
//--------------------------

#include "ft2build.h"

#include <hb-ft.h>
#include <hb-ot.h>
#include <map>
#include <algorithm>


#include "ofGraphics.h"

static int ttfGlobalDpi = 96;

//--------------------------------------------------

typedef struct {
	string filename;
	string scriptTagName;
	float fontsize;
	float baseLine;
	FT_Face face;
	hb_font_t*hbfont;
	int unicodeRangeStart;
	int unicodeRangeEnd;
	unsigned char hasUnicodeRange;
	hb_script_t script;
	hb_direction_t horiDirection;
} ul2_face_info;

typedef struct {
	unsigned char hasFace;
	unsigned char breakable;
	int cy;
	int character;
	float x_offset;
	float y_offset;
	float x_advance;
	float y_advance;
	float x1,y1,x2,y2;
} ul2_string_layouts_info;

typedef struct {
	unsigned char hasFace;
	float x1,x2,y1,y2;
	float t1,t2,v1,v2;
	float vx_offset;
	float vy_offset;
} ul2_char_layouts_info;

typedef struct {
	int codepoint;
	unsigned int face;
} ul2_face_codepoint;

typedef struct {
	hb_buffer_t*buffer;
	hb_glyph_position_t*pos;
	hb_glyph_info_t*info;
	unsigned int size;
	unsigned int index;
} ul2_face_buffer_info;

typedef enum {
	UL2_RRAW_SHAPE = 10 ,
	UL2_DRAW_TEXTURE ,
	UL2_GET_SHAPES ,
	UL2_GET_BOXES ,
	UL2_GET_BOUNDINGBOX ,
	UL2_GET_LINE_WIDTH,
	UL2_GET_LAYOUTS
} ul2_rendering_types;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Unicode wchar utils ,thanx by ofxTrueTypeFontUC
//https://github.com/hironishihara/ofxTrueTypeFontUC

#define NOT_END_OF_LINE_CHARS L"${£¥＄（［｢￡￥([｛〔〈《「『【〘〖〝‘“｟«—…‥〳〴〵"
#define NOT_START_OF_LINE_CHARS L"%}¢°‰′″℃゛゜ゝゞヽヾ！％），．：；？］｡｣､･ｧｨｩｪｫｬｭｮｯｰﾞﾟ￠,)]｝、〕〉》」』】〙〗〟’”｠»ーァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゕゖㇰㇱㇲㇳㇴㇵㇶㇸㇹㇷㇷ゚ㇺㇻㇼㇽㇾㇿ々〻‐゠–〜～　 ?!‼⁇⁈⁉・:;/。.\n"
#define BREAKABLE_CHARS NOT_START_OF_LINE_CHARS

#ifdef TARGET_WIN32
#include <locale>
#endif

namespace ul2_ttf_utils{
    
#ifdef TARGET_WIN32
	typedef basic_string<uint32_t> ustring;
#else
	typedef wstring ustring;
#endif
    
    template <class T>
    wstring convToUCS4(basic_string<T> src) {
        wstring dst = L"";
        // convert UTF-8 on char or wchar_t to UCS-4 on wchar_t
        int size = src.size();
        int index = 0;
        while (index < size) {
            wchar_t c = (unsigned char)src[index];
            if (c < 0x80) {
                dst += (c);
            }else if (c < 0xe0) {
                if (index + 1 < size) {
                    dst += (((c & 0x1f) << 6) | (src[index+1] & 0x3f));
                    index++;
                }
            }else if (c < 0xf0) {
                if (index + 2 < size) {
                    dst += (((c & 0x0f) << 12) | ((src[index+1] & 0x3f) << 6) |
                            (src[index+2] & 0x3f));
                    index += 2;
                }
            }else if (c < 0xf8) {
                if (index + 3 < size) {
                    dst += (((c & 0x07) << 18) | ((src[index+1] & 0x3f) << 12) |
                            ((src[index+2] & 0x3f) << 6) | (src[index+3] & 0x3f));
                    index += 3;
                }
            }else if (c < 0xfc) {
                if (index + 4 < size) {
                    dst += (((c & 0x03) << 24) | ((src[index+1] & 0x3f) << 18) |
                            ((src[index+2] & 0x3f) << 12) | ((src[index+3] & 0x3f) << 6) |
                            (src[index+4] & 0x3f));
                    index += 4;
                }
            }else if (c < 0xfe) {
                if (index + 5 < size) {
                    dst += (((c & 0x01) << 30) | ((src[index+1] & 0x3f) << 24) |
                            ((src[index+2] & 0x3f) << 18) | ((src[index+3] & 0x3f) << 12) |
                            ((src[index+4] & 0x3f) << 6) | (src[index+5] & 0x3f));
                    index += 5;
                }
            }
            index++;
        }
        return dst;
    }
    
#ifdef TARGET_WIN32
    ustring convUTF16ToUCS4(wstring src) {
        // decord surrogate pairs
        ustring dst;
        for (std::wstring::iterator iter=src.begin(); iter!=src.end(); ++iter) {
            if (0xD800 <=*iter && *iter<=0xDFFF) {
                if (0xD800<=*iter && *iter<=0xDBFF && 0xDC00<=*(iter+1) && *(iter+1)<=0xDFFF) {
                    int hi = *iter & 0x3FF;
                    int lo = *(iter+1) & 0x3FF;
                    int code = (hi << 10) | lo;
                    dst += code + 0x10000;
                    ++iter;
                }else {
                    // ofLog(OF_LOG_ERROR, "util::ofxTrueTypeFontUC::convUTF16ToUCS4 - wrong input" );
                }
            }else dst += *iter;
        }
        return dst;
    }
#endif
    wstring convToWString(string src) {
        
#ifdef TARGET_WIN32
        wstring dst = L"";
        typedef codecvt<wchar_t, char, mbstate_t> codecvt_t;
        
        locale loc = locale("");
        if(!std::has_facet<codecvt_t>(loc))
            return dst;
        
        const codecvt_t & conv = use_facet<codecvt_t>(loc);
        
        const std::size_t size = src.length();
        std::vector<wchar_t> dst_vctr(size);
        
        if (dst_vctr.size() == 0)
            return dst;
        
        wchar_t * const buf = &dst_vctr[0];
        
        const char * dummy;
        wchar_t * next;
        mbstate_t state = {0};
        const char * const s = src.c_str();
        
        if (conv.in(state, s, s + size, dummy, buf, buf + size, next) == codecvt_t::ok)
            dst = std::wstring(buf, next - buf);
        
        return dst;
#elif defined __clang__
        wstring dst = L"";
        for (int i=0; i<src.size(); ++i)
            dst += src[i];
#if defined(__clang_major__) && (__clang_major__ >= 4)
        dst = convToUCS4<wchar_t>(dst);
#endif
        return dst;
#else
        return convToUCS4<char>(src);
#endif
    }
    
    ustring convertTTFwstring(wstring s){
#ifdef TARGET_WIN32
        return convUTF16ToUCS4(s);
#elif defined(__clang_major__) && (__clang_major__ <= 3)
        return convToUCS4<wchar_t>(s);
#else
        return s;
#endif
    }
    
    bool isCJK(int unicode){
        return (
                (unicode>=0x4e00  && unicode<=0x9fcf)  || // CJK統合漢字
                (unicode>=0x3400  && unicode<=0x4dbf)  || // CJK統合漢字拡張A
                (unicode>=0x20000 && unicode<=0x2a6df) || // CJK統合漢字拡張B
                (unicode>=0xf900  && unicode<=0xfadf)  || // CJK互換漢字
                (unicode>=0x2f800 && unicode<=0x2fa1f) || // CJK互換漢字補助
                (unicode>=0x3190 && unicode<=0x319f)   || // 漢文用の記号
                (unicode>=0x3040 && unicode<=0x309f)   || // ひらがな
                (unicode>=0x30a0 && unicode<=0x30ff)   || // カタカナ
                (unicode>=0xff61 && unicode<=0xff9f )     // 半角カタカナ
                ) ;
    }
    
    static const ustring &SetNotEOL    = convertTTFwstring(NOT_END_OF_LINE_CHARS);
    static const ustring &SetNotSOL    = convertTTFwstring(NOT_START_OF_LINE_CHARS);
    static const ustring &SetBreakable = convertTTFwstring(BREAKABLE_CHARS);
    
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//code by OF_0.8.0
static bool librariesInitialized = false;
static FT_Library library;
static bool printVectorInfo = false;

//--------------------------------------------------------
static ofPath makeContoursForCharacter(FT_Face &face);

/*
static ofPath makeContoursForCharacter(FT_Face &face){
    
    //int num			= face->glyph->outline.n_points;
    int nContours	= face->glyph->outline.n_contours;
    int startPos	= 0;
    
    char * tags		= face->glyph->outline.tags;
    FT_Vector * vec = face->glyph->outline.points;
    
    ofPath charOutlines;
    charOutlines.setUseShapeColor(false);
    
    for(int k = 0; k < nContours; k++){
        if( k > 0 ){
            startPos = face->glyph->outline.contours[k-1]+1;
        }
        int endPos = face->glyph->outline.contours[k]+1;
        
        if(printVectorInfo){
            ofLogNotice("ofTrueTypeFont") << "--NEW CONTOUR";
        }
        
        //vector <ofPoint> testOutline;
        ofPoint lastPoint;
        
        for(int j = startPos; j < endPos; j++){
            
            if( FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_ON ){
                lastPoint.set((float)vec[j].x, (float)-vec[j].y, 0);
                if(printVectorInfo){
                    ofLogNotice("ofTrueTypeFont") << "flag[" << j << "] is set to 1 - regular point - " << lastPoint.x <<  lastPoint.y;
                }
                charOutlines.lineTo(lastPoint/64);
                
            }else{
                if(printVectorInfo){
                    ofLogNotice("ofTrueTypeFont") << "flag[" << j << "] is set to 0 - control point";
                }
                
                if( FT_CURVE_TAG(tags[j]) == FT_CURVE_TAG_CUBIC ){
                    if(printVectorInfo){
                        ofLogNotice("ofTrueTypeFont") << "- bit 2 is set to 2 - CUBIC";
                    }
                    
                    int prevPoint = j-1;
                    if( j == 0){
                        prevPoint = endPos-1;
                    }
                    
                    int nextIndex = j+1;
                    if( nextIndex >= endPos){
                        nextIndex = startPos;
                    }
                    
                    ofPoint nextPoint( (float)vec[nextIndex].x,  -(float)vec[nextIndex].y );
                    
                    //we need two control points to draw a cubic bezier
                    bool lastPointCubic =  ( FT_CURVE_TAG(tags[prevPoint]) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG(tags[prevPoint]) == FT_CURVE_TAG_CUBIC);
                    
                    if( lastPointCubic ){
                        ofPoint controlPoint1((float)vec[prevPoint].x,	(float)-vec[prevPoint].y);
                        ofPoint controlPoint2((float)vec[j].x, (float)-vec[j].y);
                        ofPoint nextPoint((float) vec[nextIndex].x,	-(float) vec[nextIndex].y);
                        
                        //cubic_bezier(testOutline, lastPoint.x, lastPoint.y, controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, nextPoint.x, nextPoint.y, 8);
                        charOutlines.bezierTo(controlPoint1.x/64.0f, controlPoint1.y/64.0f, controlPoint2.x/64.0f, controlPoint2.y/64.0f, nextPoint.x/64.0f, nextPoint.y/64.0f);
                    }
                    
                }else{
                    
                    ofPoint conicPoint( (float)vec[j].x,  -(float)vec[j].y );
                    
                    if(printVectorInfo){
                        ofLogNotice("ofTrueTypeFont") << "- bit 2 is set to 0 - conic- ";
                        ofLogNotice("ofTrueTypeFont") << "--- conicPoint point is " << conicPoint.x << conicPoint.y;
                    }
                    
                    //If the first point is connic and the last point is connic then we need to create a virutal point which acts as a wrap around
                    if( j == startPos ){
                        bool prevIsConnic = (  FT_CURVE_TAG( tags[endPos-1] ) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG( tags[endPos-1]) != FT_CURVE_TAG_CUBIC );
                        
                        if( prevIsConnic ){
                            ofPoint lastConnic((float)vec[endPos - 1].x, (float)-vec[endPos - 1].y);
                            lastPoint = (conicPoint + lastConnic) / 2.0f;
                            
                            if(printVectorInfo){
                                ofLogNotice("ofTrueTypeFont") << "NEED TO MIX WITH LAST";
                                ofLogNotice("ofTrueTypeFont") << "last is " << lastPoint.x << " " << lastPoint.y;
                            }
                        }
                    }
                    
                    //bool doubleConic = false;
                    
                    int nextIndex = j+1;
                    if( nextIndex >= endPos){
                        nextIndex = startPos;
                    }
                    
                    ofPoint nextPoint( (float)vec[nextIndex].x,  -(float)vec[nextIndex].y );
                    
                    if(printVectorInfo){
                        ofLogNotice("ofTrueTypeFont") << "--- last point is " << lastPoint.x << " " <<  lastPoint.y;
                    }
                    
                    bool nextIsConnic = (  FT_CURVE_TAG( tags[nextIndex] ) != FT_CURVE_TAG_ON ) && ( FT_CURVE_TAG( tags[nextIndex]) != FT_CURVE_TAG_CUBIC );
                    
                    //create a 'virtual on point' if we have two connic points
                    if( nextIsConnic ){
                        nextPoint = (conicPoint + nextPoint) / 2;
                        if(printVectorInfo){
                            ofLogNotice("ofTrueTypeFont") << "|_______ double connic!";
                        }
                    }
                    if(printVectorInfo){
                        ofLogNotice("ofTrueTypeFont") << "--- next point is " << nextPoint.x << " " << nextPoint.y;
                    }
                    
                    //quad_bezier(testOutline, lastPoint.x, lastPoint.y, conicPoint.x, conicPoint.y, nextPoint.x, nextPoint.y, 8);
                    charOutlines.quadBezierTo(lastPoint.x/64.0f, lastPoint.y/64.0f, conicPoint.x/64.0f, conicPoint.y/64.0f, nextPoint.x/64.0f, nextPoint.y/64.0f);
                    
                    if( nextIsConnic ){
                        lastPoint = nextPoint;
                    }
                }
            }
            
			//end for
        }
        charOutlines.close();
    }
    
	return charOutlines;
}
*/

static ofPath makeContoursForCharacter(FT_Face &face){
	bool vFlip=true;
	double x1, y1, x2, y2, x3, y3,x4,y4;
	const FT_Outline* outline= &face->glyph->outline;

    ofPath charOutlines;
    charOutlines.setUseShapeColor(false);

    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    FT_Error    error;

    FT_Int   n;         /* index of contour in outline     */
    FT_UInt  first;     /* index of first point in contour */
    FT_Int   tag;       /* current point's state           */

    if ( !outline )return charOutlines;

    first = 0;

	for (n = 0; n < outline->n_contours; n++) {
		int  last;  // index of last point in contour

		last  = outline->contours[n];
		limit = outline->points + last;

		v_start = outline->points[first];
		v_last  = outline->points[last];

		v_control = v_start;

		point = outline->points + first;
		tags  = outline->tags  + first;
		tag   = FT_CURVE_TAG(tags[0]);

		// A contour cannot start with a cubic control point!
		if (tag == FT_CURVE_TAG_CUBIC)
			return charOutlines;

		// check first point to determine origin
		if ( tag == FT_CURVE_TAG_CONIC) {
			// first point is conic control.  Yes, this happens.
			if (FT_CURVE_TAG(outline->tags[last]) == FT_CURVE_TAG_ON) {
				// start at last point if it is on the curve
				v_start = v_last;
				limit--;
			} else {
				// if both first and last points are conic,
				// start at their middle and record its position
				// for closure
				v_start.x = (v_start.x + v_last.x) / 2;
				v_start.y = (v_start.y + v_last.y) / 2;

				v_last = v_start;
			}
			point--;
			tags--;
		}
		x1 = v_start.x/64.0f;
		y1 = v_start.y/64.0f;
		if (vFlip) y1 = -y1;
		charOutlines.moveTo(x1,y1);
		x4=x1;y4=y1;


		while(point < limit) {
			point++;
			tags++;

			tag = FT_CURVE_TAG(tags[0]);
			switch(tag) {
				case FT_CURVE_TAG_ON: { // emit a single line_to
					x1 = point->x/64.0f;
					y1 = point->y/64.0f;
					if (vFlip) y1 = -y1;
					charOutlines.lineTo(x1,y1);
					x4=x1;y4=y1;

					continue;
				}

				case FT_CURVE_TAG_CONIC: { // consume conic arcs
					v_control.x = point->x;
					v_control.y = point->y;

				Do_Conic:
					if (point < limit) {
						FT_Vector vec;
						FT_Vector v_middle;

						point++;
						tags++;
						tag = FT_CURVE_TAG(tags[0]);

						vec.x = point->x;
						vec.y = point->y;

						if (tag == FT_CURVE_TAG_ON) {
							x1 = v_control.x/64.0f;
							y1 = v_control.y/64.0f;
							x2 = vec.x/64.0f;
							y2 = vec.y/64.0f;
							if (vFlip) { y1 = -y1; y2 = -y2; }
							charOutlines.quadBezierTo(x4,y4,x1,y1,x2,y2);
							x4=x2;y4=y2;
							
							continue;
						}

						if (tag != FT_CURVE_TAG_CONIC)return charOutlines;

						v_middle.x = (v_control.x + vec.x) / 2.0f;
						v_middle.y = (v_control.y + vec.y) / 2.0f;

						x1 = v_control.x/64.0f;
						y1 = v_control.y/64.0f;
						x2 = v_middle.x/64.0f;
						y2 = v_middle.y/64.0f;
						if (vFlip) { y1 = -y1; y2 = -y2; }
						charOutlines.quadBezierTo(x4,y4,x1,y1,x2,y2);
						x4=x2;y4=y2;

						v_control = vec;
						goto Do_Conic;
					}

					x1 = v_control.x/64.0f;
					y1 = v_control.y/64.0f;
					x2 = v_start.x/64.0f;
					y2 = v_start.y/64.0f;
					if (vFlip) { y1 = -y1; y2 = -y2; }
					charOutlines.quadBezierTo(x4,y4,x1,y1,x2,y2);
					x4=x2;y4=y2;

					goto Close;
				}

				default: { // FT_CURVE_TAG_CUBIC
					FT_Vector vec1, vec2;

					if (point + 1 > limit || FT_CURVE_TAG(tags[1]) != FT_CURVE_TAG_CUBIC)
						return charOutlines;

					vec1.x = point[0].x;
					vec1.y = point[0].y;
					vec2.x = point[1].x;
					vec2.y = point[1].y;

					point += 2;
					tags  += 2;

					if (point <= limit) {
						FT_Vector vec;

						vec.x = point->x;
						vec.y = point->y;

						x1 = vec1.x/64.0f;
						y1 = vec1.y/64.0f;
						x2 = vec2.x/64.0f;
						y2 = vec2.y/64.0f;
						x3 = vec.x/64.0f;
						y3 = vec.y/64.0f;
						if (vFlip) { y1 = -y1; y2 = -y2; y3 = -y3; }
						charOutlines.bezierTo(x1,y1,x2,y2,x3,y3);
						x4=x3;y4=y3;

						continue;
					}

					x1 = vec1.x/64.0f;
					y1 = vec1.y/64.0f;
					x2 = vec2.x/64.0f;
					y2 = vec2.y/64.0f;
					x3 = v_start.x/64.0f;
					y3 = v_start.y/64.0f;
					if (vFlip) { y1 = -y1; y2 = -y2; y3 = -y3; }
					charOutlines.bezierTo(x1,y1,x2,y2,x3,y3);
					x4=x3;y4=y3;

					goto Close;
				}
			}
		}
		charOutlines.close();

Close:
		first = last + 1;
	}

	return charOutlines;
}



#ifdef TARGET_OSX
static string osxFontPathByName( string fontname ){
	CFStringRef targetName = CFStringCreateWithCString(NULL, fontname.c_str(), kCFStringEncodingUTF8);
	CTFontDescriptorRef targetDescriptor = CTFontDescriptorCreateWithNameAndSize(targetName, 0.0);
	CFURLRef targetURL = (CFURLRef) CTFontDescriptorCopyAttribute(targetDescriptor, kCTFontURLAttribute);
	string fontPath = "";
	
	if(targetURL) {
		UInt8 buffer[PATH_MAX];
		CFURLGetFileSystemRepresentation(targetURL, true, buffer, PATH_MAX);
		fontPath = string((char *)buffer);
		CFRelease(targetURL);
	}
	
	CFRelease(targetName);
	CFRelease(targetDescriptor);
    
	return fontPath;
}
#endif

#ifdef TARGET_WIN32
#include <map>
// font font face -> file name name mapping
static map<string, string> fonts_table;
// read font linking information from registry, and store in std::map
static void initWindows(){
	LONG l_ret;

	const wchar_t *Fonts = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

	HKEY key_ft;
	l_ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, Fonts, 0, KEY_QUERY_VALUE, &key_ft);
	if (l_ret != ERROR_SUCCESS){
	    ofLogError("ofTrueTypeFont") << "initWindows(): couldn't find fonts registery key";
        return;
	}

	DWORD value_count;
	DWORD max_data_len;
	wchar_t value_name[2048];
	BYTE *value_data;


	// get font_file_name -> font_face mapping from the "Fonts" registry key

	l_ret = RegQueryInfoKeyW(key_ft, NULL, NULL, NULL, NULL, NULL, NULL, &value_count, NULL, &max_data_len, NULL, NULL);
	if(l_ret != ERROR_SUCCESS){
	    ofLogError("ofTrueTypeFont") << "initWindows(): couldn't query registery for fonts";
        return;
	}

	// no font installed
	if (value_count == 0){
	    ofLogError("ofTrueTypeFont") << "initWindows(): couldn't find any fonts in registery";
        return;
	}

	// max_data_len is in BYTE
	value_data = static_cast<BYTE *>(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, max_data_len));
	if(value_data == NULL) return;

	char value_name_char[2048];
	char value_data_char[2048];
	/*char ppidl[2048];
	char fontsPath[2048];
    SHGetKnownFolderIDList(FOLDERID_Fonts, 0, NULL, &ppidl);
    SHGetPathFromIDList(ppidl,&fontsPath);*/
    string fontsDir = getenv ("windir");
    fontsDir += "\\Fonts\\";
	for (DWORD i = 0; i < value_count; ++i)
	{
			DWORD name_len = 2048;
			DWORD data_len = max_data_len;

			l_ret = RegEnumValueW(key_ft, i, value_name, &name_len, NULL, NULL, value_data, &data_len);
			if(l_ret != ERROR_SUCCESS){
			     ofLogError("ofTrueTypeFont") << "initWindows(): couldn't read registry key for font type";
			     continue;
			}

            wcstombs(value_name_char,value_name,2048);
			wcstombs(value_data_char,reinterpret_cast<wchar_t *>(value_data),2048);
			string curr_face = value_name_char;
			string font_file = value_data_char;
			curr_face = curr_face.substr(0, curr_face.find('(') - 1);
			fonts_table[curr_face] = fontsDir + font_file;
	}


	HeapFree(GetProcessHeap(), 0, value_data);

	l_ret = RegCloseKey(key_ft);
}

static string winFontPathByName( string fontname ){
    if(fonts_table.find(fontname)!=fonts_table.end()){
        return fonts_table[fontname];
    }
    for(map<string,string>::iterator it = fonts_table.begin(); it!=fonts_table.end(); it++){
        if(ofIsStringInString(ofToLower(it->first),ofToLower(fontname))) return it->second;
    }
    return "";
}
#endif

#ifdef TARGET_LINUX
static string linuxFontPathByName(string fontname){
	string filename;
	FcPattern * pattern = FcNameParse((const FcChar8*)fontname.c_str());
	FcBool ret = FcConfigSubstitute(0,pattern,FcMatchPattern);
	if(!ret){
		ofLogError() << "linuxFontPathByName(): couldn't find font file or system font with name \"" << fontname << "\"";
		return "";
	}
	FcDefaultSubstitute(pattern);
	FcResult result;
	FcPattern * fontMatch=NULL;
	fontMatch = FcFontMatch(0,pattern,&result);
    
	if(!fontMatch){
		ofLogError() << "linuxFontPathByName(): couldn't match font file or system font with name \"" << fontname << "\"";
		return "";
	}
	FcChar8	*file;
	if (FcPatternGetString (fontMatch, FC_FILE, 0, &file) == FcResultMatch){
		filename = (const char*)file;
	}else{
		ofLogError() << "linuxFontPathByName(): couldn't find font match for \"" << fontname << "\"";
		return "";
	}
	return filename;
}
#endif



static bool loadFontFace(string fontname, int _fontSize, FT_Face & face, string & filename){
	filename = ofToDataPath(fontname,true);
	ofFile fontFile(filename,ofFile::Reference);
	int fontID = 0;
	if(!fontFile.exists()){
#ifdef TARGET_LINUX
		filename = linuxFontPathByName(fontname);
#elif defined(TARGET_OSX)
		if(fontname==OF_TTF_SANS){
			fontname = "Helvetica Neue";
			fontID = 4;
		}else if(fontname==OF_TTF_SERIF){
			fontname = "Times New Roman";
		}else if(fontname==OF_TTF_MONO){
			fontname = "Menlo Regular";
		}
		filename = osxFontPathByName(fontname);
#elif defined(TARGET_WIN32)
		if(fontname==OF_TTF_SANS){
			fontname = "Arial";
		}else if(fontname==OF_TTF_SERIF){
			fontname = "Times New Roman";
		}else if(fontname==OF_TTF_MONO){
			fontname = "Courier New";
		}
        filename = winFontPathByName(fontname);
#endif
		if(filename == "" ){
			ofLogError("ofTrueTypeFont") << "loadFontFace(): couldn't find font \"" << fontname << "\"";
			return false;
		}
		ofLogVerbose("ofTrueTypeFont") << "loadFontFace(): \"" << fontname << "\" not a file in data loading system font from \"" << filename << "\"";
	}
	FT_Error err;
	err = FT_New_Face( library, filename.c_str(), fontID, &face );
	if (err) {
		// simple error table in lieu of full table (see fterrors.h)
		string errorString = "unknown freetype";
		if(err == 1) errorString = "INVALID FILENAME";
		ofLogError("ofTrueTypeFont") << "loadFontFace(): couldn't create new face for \"" << fontname << "\": FT_Error " << err << " " << errorString;
		return false;
	}
    
	return true;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ofxTrueTypeFontUL2::Impl
class ofxTrueTypeFontUL2::Impl {
public:
	Impl();
	~Impl() {};
    
	bool implLoadFont(string filename, float fontsize, bool _bAntiAliased, bool makeContours, float _simplifyAmt, int dpi,bool useTexture,string scriptTagName);
	bool implLoadSubFont(string filename,float sizeRate, float baseLineRate ,int unicodeRangeStart,int unicodeRangeEnd,string scriptTagName);
	bool implLoadSubFontVal(string filename,float fontsize, float baseLine ,int unicodeRangeStart,int unicodeRangeEnd,string scriptTagName);
    
	void implUnloadFont();
    
	vector <ofPath> charOutlines;
	

	vector<ul2_char_layouts_info> cps;  // properties for each character
    
	void drawChar(int c, float x, float y);
	void drawCharAsShape(int c, float x, float y);
    
  	vector<ofTexture> textures;
	ofMesh stringQuads;
    
	int getCharID(const int codepoint,const unsigned int faceId);
	void loadChar(const int & charID);
    
	vector<ul2_face_info>  faces;
	vector<ul2_string_layouts_info> getHbPosition(wstring wsrc);
	map<wstring,vector<ul2_string_layouts_info> > hbPositionCache;
	vector<ul2_face_codepoint> loadedChars;
	
	template <class T>
	void commonLayouts(wstring src ,float x, float y,float width,float height,int textAlign,ul2_rendering_types type ,T &result);
	template <class T>
	void commonLayouts2(wstring src ,float x, float y,float width,float height,ul2_text_align textAlign,vector<ofRectangle> *lineWidthList,ul2_rendering_types type ,T &result);
    
	void addOTFeature(const char*feature_tag,unsigned int value,unsigned int  start=0,unsigned int end=static_cast<unsigned>(-1));
	void removeOTFeature(const char*feature_tag);
	void clearOTFeatures();
	void printOTFeatures();
    
	bool bLoadedOk_;
	bool bAntiAliased_;
	bool bUseTexture_;
	float simplifyAmt_;
	int dpi_;
	float lineHeight_;
	float letterSpacing_;
	float spaceSize_;
	float baseFontSize_;
	bool bMakeContours_;
	int	border_;  // visibleBorder;
	hb_direction_t m_direction;
	hb_direction_t m_subDirection;
	unsigned int m_featuresnum;
	hb_feature_t *m_features;
	bool bUseVrt2Layout; // added 'vrt2',
	bool bUseProportional; // added 'palt' or 'vpal'
	bool bUseLayoutCache;
	bool bAlignByPixel;
	bool bWordWrap;
	bool bWritingHorizontal;
	float strokeWidth;
    
#ifdef TARGET_OPENGLES
	GLint blend_src, blend_dst;
	GLboolean blend_enabled;
	GLboolean texture_2d_enabled;
#endif
};

ofxTrueTypeFontUL2::Impl::Impl(){
	bLoadedOk_=false;
	bAntiAliased_=false;
	
	letterSpacing_=0.0f;
	spaceSize_=1.0f;
	stringQuads.setMode(OF_PRIMITIVE_TRIANGLES);
    
    
	// 3 pixel border around the glyph
	// We show 2 pixels of this, so that blending looks good.
	// 1 pixels is hidden because we don't want to see the real edge of the texture
    
	border_ = 3;
	m_direction=HB_DIRECTION_LTR;
	m_subDirection = HB_DIRECTION_TTB;
	bWritingHorizontal=true;
    
	m_featuresnum=0;
	m_features=NULL;
	
	bUseVrt2Layout=false;
	bUseProportional=false;
	bUseLayoutCache=false;
    lineHeight_=0;
	bAlignByPixel=false;
	bWordWrap=true;
}

bool ofxTrueTypeFontUL2::Impl::implLoadFont(string filename, float fontsize, bool bAntiAliased, bool makeContours, float simplifyAmt, int dpi,bool useTexture,string scriptTagName) {
	bMakeContours_ = makeContours;
	bUseTexture_ = useTexture;
    
	if (bLoadedOk_ == true)   implUnloadFont();
	bLoadedOk_ = false;
    
	/*
     FT_Error err = FT_Init_FreeType(&library);
     if (err) {
     ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::loadFont - Error initializing freetype lib: FT_Error = %d", err);
     return false;
     }
     */
    
	dpi_ = dpi;
	if( dpi_ == 0 )dpi_ = ttfGlobalDpi;
    
	bAntiAliased_ = bAntiAliased;
	simplifyAmt_ = simplifyAmt;
	baseFontSize_ = fontsize;
	if(lineHeight_==0)lineHeight_ = baseFontSize_ * 2.2f;
	return implLoadSubFontVal(filename,baseFontSize_,0,0,0,scriptTagName);
}

void ofxTrueTypeFontUL2::Impl::implUnloadFont(){
	cps.clear();
	textures.clear();
	loadedChars.clear();
	charOutlines.clear();
	hbPositionCache.clear();
    
    if(!bLoadedOk_)return;
    // ------------- close the library and typeface
	for(int i=0;i<faces.size();i++){
		hb_font_destroy(faces[i].hbfont);
		FT_Done_Face(faces[i].face);
	}
	faces.clear();
	//FT_Done_FreeType(library);
    
	if(m_featuresnum>0)free(m_features);
	m_featuresnum=0;
	bLoadedOk_ = false;
}

bool ofxTrueTypeFontUL2::Impl::implLoadSubFontVal(string filename,float fontsize, float baseLine,int unicodeRangeStart,int unicodeRangeEnd,string scriptTagName){
    
	ul2_face_info set;
	//set.filename=filename;
	set.fontsize=fontsize;
	set.baseLine=baseLine;
	set.unicodeRangeEnd=unicodeRangeEnd;
	set.unicodeRangeStart=unicodeRangeStart;
	set.hasUnicodeRange=set.unicodeRangeStart<=set.unicodeRangeEnd&&(set.unicodeRangeStart!=0||set.unicodeRangeEnd!=0);
	set.scriptTagName=scriptTagName;
    
	/*
     FT_Error err;
     err = FT_New_Face(library, ofToDataPath(set.filename).c_str(), 0, &set.face);
     if (err) {
     // simple error table in lieu of full table (see fterrors.h)
     string errorString = "unknown freetype";
     if (err == 1)errorString = "INVALID FILENAME";
     ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::loadFont - %s: %s: FT_Error = %d", errorString.c_str(), filename.c_str(), err);
     return false;
     }
     */
    
	if(!loadFontFace(filename,fontsize,set.face,set.filename)){
		return false;
	}
    
	FT_Set_Char_Size(set.face, ceil( set.fontsize * 64.0f), ceil( set.fontsize * 64.0f) , dpi_, dpi_);
	set.hbfont = hb_font_reference(hb_ft_font_create( set.face, 0 ));
	hb_ft_font_set_funcs( set.hbfont );
	
	//Set default script tags.
	if(!set.scriptTagName.empty()){
		set.script=hb_script_from_string(set.scriptTagName.c_str(),set.scriptTagName.length());
	}else{
		set.script = HB_SCRIPT_INVALID;
		hb_face_t *face = hb_font_get_face(set.hbfont);
		hb_tag_t* listGSub = NULL;
		hb_tag_t* listGPOS = NULL;
		unsigned int cntGSUB = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, NULL, NULL);
		if(cntGSUB>0){
			listGSub = (hb_tag_t*) malloc(cntGSUB * sizeof(hb_tag_t));
			hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &cntGSUB, listGSub);
		}
		unsigned int cntGPOS = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GPOS, 0, NULL, NULL);
		if(cntGPOS>0){
			listGPOS = (hb_tag_t*) malloc(cntGPOS * sizeof(hb_tag_t));
			hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &cntGPOS, listGPOS);
		}
		if(cntGSUB>0||cntGPOS>0){
			set.script = hb_script_from_iso15924_tag(cntGSUB>cntGPOS?*listGSub:*listGPOS);
		}
		if(cntGSUB>0)free(listGSub);
		if(cntGPOS>0)free(listGPOS);
	}
    
	// scrict direction (not using)
	set.horiDirection = hb_script_get_horizontal_direction (set.script);
	faces.push_back(set);

	bLoadedOk_=true;
	return bLoadedOk_;
}

bool ofxTrueTypeFontUL2::Impl::implLoadSubFont(string filename,float sizeRate, float baseLineRate,int unicodeRangeStart,int unicodeRangeEnd,string scriptTagName){
	return implLoadSubFontVal(filename,baseFontSize_*sizeRate,baseFontSize_*baseLineRate,unicodeRangeStart,unicodeRangeEnd,scriptTagName);
}

void ofxTrueTypeFontUL2::Impl::addOTFeature(const char*feature_tag,unsigned int value,unsigned int  start,unsigned int end){
	hb_tag_t add=hb_tag_from_string(feature_tag,-1);
	for(int i=0;i<m_featuresnum;i++){
		if((m_features+i)->tag==add)return;
	}
	hb_feature_t* newf=(hb_feature_t*)malloc(sizeof(hb_feature_t)*(m_featuresnum+1));
	if(m_featuresnum>0){
		memcpy(newf,m_features,sizeof(hb_feature_t)*m_featuresnum);
		free(m_features);
	}
	newf[m_featuresnum].end=end;
	newf[m_featuresnum].start=start;
	newf[m_featuresnum].value=value;
	newf[m_featuresnum].tag=add;
	m_featuresnum++;
	m_features=newf;
}
void ofxTrueTypeFontUL2::Impl::removeOTFeature(const char*feature_tag){
	if(m_featuresnum==0)return;
	hb_tag_t rem=hb_tag_from_string(feature_tag,-1);
	unsigned int c=0;
	for(int i=0;i<m_featuresnum;i++){
		if((m_features+i)->tag!=rem)c++;
	}
	hb_feature_t* newf=(hb_feature_t*)malloc(sizeof(hb_feature_t)*c);
	int t=0;
	for(int i=0;i<m_featuresnum;i++){
		if((m_features+i)->tag!=rem){
			(newf+t)->tag=(m_features+i)->tag;
			(newf+t)->value=(m_features+i)->value;
			(newf+t)->start=(m_features+i)->start;
			(newf+t)->end=(m_features+i)->end;
			t++;
		}
	}
	free(m_features);
	m_featuresnum=c;
	m_features=newf;
}
void ofxTrueTypeFontUL2::Impl::clearOTFeatures(){
	if(m_featuresnum>0)free(m_features);
	m_featuresnum=0;
}

void ofxTrueTypeFontUL2::Impl::printOTFeatures(){
	for(int i=0;i<m_featuresnum;i++){
		char p[128];
		hb_feature_to_string((m_features+i),p,128 );
		printf("%i : %s , %i ( %i - %i ) \n",i,p,(m_features+i)->value,(m_features+i)->start,(m_features+i)->end );
	}
}

vector<ul2_string_layouts_info> ofxTrueTypeFontUL2::Impl::getHbPosition(wstring wsrc) {
	
	vector<ul2_string_layouts_info> result;
	if(bUseLayoutCache){
		result=hbPositionCache[wsrc];
		if(!result.empty())return result;
	}
	const ul2_ttf_utils::ustring &src=ul2_ttf_utils::convertTTFwstring(wsrc);
	const int len = (int)src.length();
	const int facenum=faces.size();
	const bool _dirNormal=m_direction==HB_DIRECTION_LTR||m_direction==HB_DIRECTION_TTB;
    
	ul2_face_buffer_info*fBuffs =(ul2_face_buffer_info*)malloc(sizeof(ul2_face_buffer_info)*facenum);
	ul2_face_buffer_info *abuff;
	int k,i,t,cy,c,kbs;
    
	for(i=0;i<facenum;i++){
		abuff=fBuffs+i;
        abuff->buffer= hb_buffer_create();
		hb_buffer_set_script(abuff->buffer,faces[i].script);
		hb_buffer_set_direction( abuff->buffer, m_direction );
		//Check this cast ,except windows and mac.
		hb_buffer_add_utf32( abuff->buffer, (const uint32_t*)src.c_str(),len,0,len);
        hb_shape( faces[i].hbfont, abuff->buffer, m_features, m_featuresnum);
		abuff->size=hb_buffer_get_length(abuff->buffer);
		const int pt=_dirNormal?0:(abuff->size-1);
		abuff->pos=hb_buffer_get_glyph_positions( abuff->buffer,NULL )+pt;
		abuff->info=hb_buffer_get_glyph_infos ( abuff->buffer, NULL )+pt;
		abuff->index=pt;
	}
    
	k=0;
	kbs=-1;
	for(i=0;i<len;){
		if(src[i]>0x0020){
			for(k=facenum-1;k>0;k--){
				if(fBuffs[k].info->codepoint!=0){
					if(faces[k].hasUnicodeRange==1){
						if(faces[k].unicodeRangeStart<=src[i]&&src[i]<=faces[k].unicodeRangeEnd)break;
					}else break;
				}
			}
		}else{
			//Calculate space size.(that between different faces)
			if(i+1<len){
				for(t=facenum-1;t>0;t--){
					if(FT_Get_Char_Index(faces[t].face,src[i+1])>0){
						if(faces[t].hasUnicodeRange==1){
							if(faces[t].unicodeRangeStart<=src[i+1]&&src[i+1]<=faces[t].unicodeRangeEnd)break;
						}else break;
					}
				}
				if(k!=t)kbs=t;
			}
		}
        
		abuff=fBuffs+k;
		cy = getCharID(abuff->info->codepoint,k);
		
		ul2_string_layouts_info v;
        
		if(kbs==-1){
			v.x_advance = abuff->pos->x_advance / 64.0f;
			v.y_advance = -abuff->pos->y_advance / 64.0f;
		}else{
			v.x_advance =   MIN(abuff->pos->x_advance,(fBuffs+kbs)->pos->x_advance) / 64.0f;
			v.y_advance = - MIN(abuff->pos->y_advance,(fBuffs+kbs)->pos->y_advance) / 64.0f;
			kbs=-1;
		}
		if(bWritingHorizontal){
			v.x_offset = abuff->pos->x_offset / 64.0f;
			v.y_offset = -abuff->pos->y_offset / 64.0f  -faces[k].baseLine;
		}else{
			//HarfBuzz should use the x_offset value by documents,
			//but value of X is deviates slightly.(on Jpanese vertical font)
			//So this zero point(X) is get by freeType values (horiAdvance*.5f);
			v.x_offset = cps[cy].vx_offset + faces[k].baseLine;
			v.y_offset = cps[cy].vy_offset - abuff->pos->y_offset / 64.0f;
		}
        
		v.x1 = cps[cy].x1;
		v.x2 = cps[cy].x2;
		v.y1 = cps[cy].y1;
		v.y2 = cps[cy].y2;
		v.cy = cy;
		v.hasFace = cps[cy].hasFace;
		v.character = src[i];
		v.breakable = (ul2_ttf_utils::SetNotEOL.find(src[i])==string::npos &&
                       (ul2_ttf_utils::isCJK(src[i])||(i<len-1?ul2_ttf_utils::isCJK(src[i+1]):true)||ul2_ttf_utils::SetBreakable.find(src[i])!=string::npos) &&
                       (i<len-1? ul2_ttf_utils::SetNotSOL.find(src[i+1])==string::npos :true))?1:0;
		result.push_back(v);
        
		//On line-break,activate default face;
		if(src[i]==L'\n')k=0;
        
		if(_dirNormal){
			if(abuff->size-1 > abuff->index){
				//Recovery for cluster values
				i=MAX((abuff->info+1)->cluster,i+1);
				for(t=0;t<facenum;t++){
                    c=fBuffs[t].info->cluster;
					while(i>c){
						if(fBuffs[t].size-1>fBuffs[t].index){
							fBuffs[t].info++;
							fBuffs[t].pos++;
							fBuffs[t].index++;
                            c=MAX(fBuffs[t].info->cluster,c+1);
						}else break;
					}
				}
			}else break;
		}else{
			if(0 < abuff->index){
				//Recovery for cluster values
				i=MAX((abuff->info-1)->cluster,i+1);
				for(t=0;t<facenum;t++){
                    c=fBuffs[t].info->cluster;
					while(i>c){
						if(0<fBuffs[t].index){
							fBuffs[t].info--;
							fBuffs[t].pos--;
							fBuffs[t].index--;
                            c=MAX(fBuffs[t].info->cluster,c+1);
						}else break;
					}
				}
			}else break;
		}
	}
    
	//Clear memory.
	for(i=0;i<facenum;i++)hb_buffer_destroy((fBuffs+i)->buffer);
	free(fBuffs);
	if(bUseLayoutCache)hbPositionCache[wsrc]=result;
	return result;
}

template<class T1,class T2>inline void _push_back(T1 &t1,T2 t2){}
template<class T>inline void _push_back(vector<T>&t1,T t2){t1.push_back(t2);}
template<class T>inline void _setRect(T &t,float x,float y,float w,float h){}
inline void _setRect(ofRectangle &t,float x,float y,float w,float h){t.x=x;t.y=y;t.width=w,t.height=h;}
template<class T>inline void _push_layout(T&t,float x,float y,int i){}
inline void _push_layout(vector<ofxFaceVec2> &t,float x,float y,int i){ofxFaceVec2 p;p.x=x;p.y=y;p.faceIndex=i; t.push_back(p);}

template<class T>
void ofxTrueTypeFontUL2::Impl::commonLayouts2(wstring src ,float x, float y,float width,float height,ul2_text_align textAlign,vector<ofRectangle> *lineWidthList,ul2_rendering_types type ,T &result){
    
	const vector<ul2_string_layouts_info> &pos=getHbPosition(src);
	const int len = pos.size();
    
	//control direction
	const float rvRTL = m_direction==HB_DIRECTION_RTL ? -1.0f: 1.0f ;
	const float rvBTT = m_direction==HB_DIRECTION_BTT ? -1.0f: 1.0f ;
    
	//letterSpacing
	const float h_letterSpace = (bWritingHorizontal? baseFontSize_ * letterSpacing_ : 0.0f ) * rvRTL;
	const float v_letterSpace = (bWritingHorizontal? 0.0f : baseFontSize_ * letterSpacing_ ) * rvBTT;
	const float h_space = spaceSize_ * rvRTL ;
	const float v_space = spaceSize_ * rvBTT ;
	const float boxWidth =  bWritingHorizontal? width:height;
    
	//
	unsigned int index	= 0;
	float X = 0;
	float Y = 0;
	unsigned int _index = 0;
	float __X = 0;
	float _Y = 0;
	float minx = 0;
	float miny = 0;
	float maxx = 0;
	float maxy = 0;
	bool bFirstCharacter = true;
	float x1,y1,x2,y2;
    
	int lineIndex=0;
	float ax,ay;
    ax=x;
	ay=y;
    
	bool lineBreak=false;
	bool setAlign=textAlign!=UL2_TEXT_ALIGN_INVALID;
    
	while (index < len) {
		//Set alignment.
		if(setAlign){
			if(textAlign==UL2_TEXT_ALIGN_LEFT){
				if(bAlignByPixel||m_direction!=HB_DIRECTION_LTR) ax=x-(*lineWidthList)[lineIndex].x;
			}else if(textAlign==UL2_TEXT_ALIGN_CENTER){
				ax=x+(width-(*lineWidthList)[lineIndex].width)*.5-(*lineWidthList)[lineIndex].x;
			}else if(textAlign==UL2_TEXT_ALIGN_RIGHT){
				if(bAlignByPixel||m_direction!=HB_DIRECTION_RTL) ax=x+width-(*lineWidthList)[lineIndex].width-(*lineWidthList)[lineIndex].x;
				else ax=x+width;
			}else if(textAlign==UL2_TEXT_ALIGN_V_TOP){
				if(bAlignByPixel||m_direction!=HB_DIRECTION_TTB) ay=y-(*lineWidthList)[lineIndex].y;
			}else if(textAlign==UL2_TEXT_ALIGN_V_MIDDLE){
				ay=y+(height-(*lineWidthList)[lineIndex].height)*.5-(*lineWidthList)[lineIndex].y;
			}else if(textAlign==UL2_TEXT_ALIGN_V_BOTTOM){
				if(bAlignByPixel||m_direction!=HB_DIRECTION_BTT) ay=y+height-(*lineWidthList)[lineIndex].height-(*lineWidthList)[lineIndex].y;
				else ay=y+height;
			}
			lineIndex++;
			setAlign=false;
		}
        
		if (pos[index].character == L'\n') {
			lineBreak=true;
		}else{
			if (!pos[index].hasFace) {
				//Update pen position.(no faces)
				X += pos[index].x_advance * h_space ;
				Y += pos[index].y_advance * v_space ;
			}else {
				//Update pen position. (reverse direction)
				X += m_direction==HB_DIRECTION_RTL ? - pos[index].x_advance : 0 ;
				Y += m_direction==HB_DIRECTION_BTT ? - pos[index].y_advance : 0 ;
                
				//------------------------------------------------------------------------------------
				x1 = X +ax + pos[index].x1+pos[index].x_offset;
				y1 = Y +ay + pos[index].y1+pos[index].y_offset;
				x2 = X +ax + pos[index].x2+pos[index].x_offset;
				y2 = Y +ay + pos[index].y2+pos[index].y_offset;
                
				if(type==UL2_GET_BOUNDINGBOX||type==UL2_GET_LINE_WIDTH){
					//Get bounding box.
					if (bFirstCharacter == true) {
						minx = x2;miny = y2;
						maxx = x1;maxy = y1;
						bFirstCharacter = false;
					} else {
						if (x2 < minx)minx = x2;
						if (y2 < miny)miny = y2;
						if (x1 > maxx)maxx = x1;
						if (y1 > maxy)maxy = y1;
					}
				}else if(type==UL2_GET_BOXES){
					//Get boxes.
					_push_back(result,ofRectangle(x2,y2,x1-x2,y1-y2));
				}else if(type==UL2_GET_SHAPES){
					//Get shapes.
					ofPath path=charOutlines[pos[index].cy];
					path.translate(ofVec3f(ax+X+pos[index].x_offset,ay+Y+pos[index].y_offset));
					if (simplifyAmt_>0)path.simplify(simplifyAmt_);
					_push_back(result,path);
				}else if(type==UL2_DRAW_TEXTURE){
					//Draw textures.
					drawChar(pos[index].cy,ax+X+pos[index].x_offset,ay+Y+pos[index].y_offset);
				}else if(type==UL2_RRAW_SHAPE){
					//Draw shapes.
					drawCharAsShape(pos[index].cy,ax + X+pos[index].x_offset,ay+ Y+pos[index].y_offset);
				}else if(type==UL2_GET_LAYOUTS){
					_push_layout(result,ax+X+pos[index].x_offset,ay+Y+pos[index].y_offset,pos[index].cy);
				}
				//------------------------------------------------------------------------------------
				//Update pen position. (regular direction)
				X += m_direction==HB_DIRECTION_RTL ? 0 : pos[index].x_advance ;
				Y += m_direction==HB_DIRECTION_BTT ? 0 : pos[index].y_advance ;
			}
			//Add letter spacing.
			X += h_letterSpace ;
			Y += v_letterSpace ;
			
			//Calculate a line-break position.
			if(boxWidth>0){
				if(!bWordWrap||pos[index].breakable){
					_index=index+1;__X=X;_Y=Y;
					while(_index<len){
						if(pos[_index].character== L'\n')break;
						__X += m_direction==HB_DIRECTION_RTL ? - pos[_index].x_advance : 0 ;
						_Y += m_direction==HB_DIRECTION_BTT ? - pos[_index].y_advance : 0 ;
						if((m_direction==HB_DIRECTION_LTR && (__X + pos[_index].x1+pos[_index].x_offset>boxWidth))||
                           (m_direction==HB_DIRECTION_RTL && (__X + pos[_index].x2+pos[_index].x_offset<-boxWidth))||
                           (m_direction==HB_DIRECTION_TTB && (_Y + pos[_index].y1+pos[_index].y_offset>boxWidth))||
                           (m_direction==HB_DIRECTION_BTT && (_Y + pos[_index].y2+pos[_index].y_offset<-boxWidth))){
							lineBreak=true;
							break;
						}else if(!bWordWrap||pos[_index].breakable)break;
						if (!pos[_index].hasFace) {
							__X += pos[_index].x_advance * h_space ;
							_Y += pos[_index].y_advance * v_space ;
						}else {
							__X += m_direction==HB_DIRECTION_RTL ? 0 : pos[_index].x_advance ;
							_Y += m_direction==HB_DIRECTION_BTT ? 0 : pos[_index].y_advance ;
						}
						__X += h_letterSpace;
						_Y += v_letterSpace;
						_index++;
					}
				}
			}
		}
		//------------------------------------------------------------------------------------
		//Update pen position. (line-break)
		if(lineBreak){
			if(m_subDirection==HB_DIRECTION_TTB){
				X = 0 ;
				Y += lineHeight_;
			}else if(m_subDirection==HB_DIRECTION_BTT){
				X = 0 ;
				Y -= lineHeight_;
			}else if(m_subDirection==HB_DIRECTION_RTL){
				X -= lineHeight_ ;
				Y = 0 ;
			}else if(m_subDirection==HB_DIRECTION_LTR){
				X += lineHeight_ ;
				Y = 0 ;
			}
			if(type==UL2_GET_LINE_WIDTH){
				bFirstCharacter=true;
				_push_back(result,ofRectangle(minx,miny,maxx-minx,maxy-miny));
			}
			setAlign=textAlign!=UL2_TEXT_ALIGN_INVALID;
			lineBreak=false;
		}
		index++;
	}
    
	//-------------------------------------------------------------------------------
	if(type==UL2_GET_BOUNDINGBOX) _setRect(result,minx,miny,maxx-minx,maxy-miny);
	if(type==UL2_GET_LINE_WIDTH) _push_back(result,ofRectangle(minx,miny,maxx-minx,maxy-miny));
	//-------------------------------------------------------------------------------
}

template<class T>
void ofxTrueTypeFontUL2::Impl::commonLayouts(wstring src ,float x, float y,float width,float height,int textAlign,ul2_rendering_types type ,T &result){
	if(textAlign!=UL2_TEXT_ALIGN_INVALID){
		ul2_text_align align=UL2_TEXT_ALIGN_INVALID;
		vector<ofRectangle> wList;
		commonLayouts2(src,0,0,width,height,UL2_TEXT_ALIGN_INVALID,NULL,UL2_GET_LINE_WIDTH,wList);
		ofRectangle vBox;
		const unsigned int size=wList.size();
		if(bAlignByPixel){
			float x1,y1,x2,y2;
			x1=wList[0].x;
			y1=wList[0].y;
			x2=wList[0].x+wList[0].width;
			y2=wList[0].y+wList[0].height;
			for(int i=1;i<size;i++){
				if(x1>wList[i].x)x1=wList[i].x;
				if(y1>wList[i].y)y1=wList[i].y;
				if(x2<wList[i].x+wList[i].width)x2=wList[i].x+wList[i].width;
				if(y2<wList[i].y+wList[i].height)y2=wList[i].y+wList[i].height;
			}
			vBox.x=x1;
			vBox.y=y1;
			vBox.width=x2-x1;
			vBox.height=y2-y1;
		}else{
			vBox.height=vBox.width=size*lineHeight_-(lineHeight_-baseFontSize_);
			if     ( m_subDirection == HB_DIRECTION_TTB ) vBox.y = -baseFontSize_;
			else if( m_subDirection == HB_DIRECTION_BTT ) vBox.y = -vBox.height;
			else if( m_subDirection == HB_DIRECTION_LTR ) vBox.x = -baseFontSize_*.5f;
			else if( m_subDirection == HB_DIRECTION_RTL ) vBox.x = -(vBox.width - baseFontSize_*.5f);
		}
		if(bWritingHorizontal){
			if     ( textAlign & UL2_TEXT_ALIGN_LEFT     ) align = UL2_TEXT_ALIGN_LEFT;
			else if( textAlign & UL2_TEXT_ALIGN_CENTER   ) align = UL2_TEXT_ALIGN_CENTER;
			else if( textAlign & UL2_TEXT_ALIGN_RIGHT    ) align = UL2_TEXT_ALIGN_RIGHT;
			if     ( textAlign & UL2_TEXT_ALIGN_V_TOP    ) y += -vBox.y;
			else if( textAlign & UL2_TEXT_ALIGN_V_MIDDLE ) y += (height-vBox.height)*.5f-vBox.y;
			else if( textAlign & UL2_TEXT_ALIGN_V_BOTTOM ) y += height-vBox.height-vBox.y;
		}else{
			if     ( textAlign & UL2_TEXT_ALIGN_V_TOP    ) align = UL2_TEXT_ALIGN_V_TOP;
			else if( textAlign & UL2_TEXT_ALIGN_V_MIDDLE ) align = UL2_TEXT_ALIGN_V_MIDDLE;
			else if( textAlign & UL2_TEXT_ALIGN_V_BOTTOM ) align = UL2_TEXT_ALIGN_V_BOTTOM;
			if     ( textAlign & UL2_TEXT_ALIGN_LEFT     ) x += -vBox.x;
			else if( textAlign & UL2_TEXT_ALIGN_CENTER   ) x += (width-vBox.width)*.5f-vBox.x;
			else if( textAlign & UL2_TEXT_ALIGN_RIGHT    ) x += width-vBox.width-vBox.x;
		}
		commonLayouts2(src,x,y,width,height,align,&wList,type,result);
	}else{
		commonLayouts2(src,x,y,width,height,UL2_TEXT_ALIGN_INVALID,NULL,type,result);
	}
}

//-----------------------------------------------------------
int ofxTrueTypeFontUL2::Impl::getCharID(const int codepoint,const unsigned int faceId) {
	int point = 0;
	for (; point != (int)loadedChars.size(); ++point) {
		if (loadedChars[point].codepoint==codepoint && loadedChars[point].face==faceId) break;
	}
	if (point == loadedChars.size()) {
		ul2_face_codepoint t;
		t.codepoint=codepoint;
		t.face=faceId;
		loadedChars.push_back(t);
		cps.push_back(ul2_char_layouts_info());
		if(bMakeContours_)charOutlines.push_back(ofPath());
		if(bUseTexture_)textures.push_back(ofTexture());
		loadChar(point);
	}
	return point;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::Impl::loadChar(const int & charID) {
	int i = charID;
	ul2_face_info &set=faces[loadedChars[i].face];
	
	//------------------------------------------ anti aliased or not:
	FT_Error err = FT_Load_Glyph( set.face, loadedChars[i].codepoint,FT_LOAD_DEFAULT);
	if(err)ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::loadFont - Error with FT_Load_Glyph %i: FT_Error = %d", loadedChars[i].codepoint , err);
    
    
	float width,height,topExtent,leftextent,vx,vy;
	width = set.face->glyph->metrics.width / 64.0f;
	height = set.face->glyph->metrics.height / 64.0f;
	topExtent  = -set.face->glyph->metrics.horiBearingY / 64.0f ;
	leftextent = set.face->glyph->metrics.horiBearingX / 64.0f ;
    
	vy=set.face->glyph->metrics.vertBearingY / 64.0f;
	vx=-set.face->glyph->metrics.horiAdvance /64.0f*.5f;
    
	cps[i].vx_offset=vx;
	cps[i].vy_offset=vy;
    
	cps[i].x1 = leftextent + width ;
	cps[i].x2 = leftextent;
	cps[i].y1 = topExtent + height;
	cps[i].y2 = topExtent;
	cps[i].hasFace=width>0&&height>0?1:0;
    
	if(cps[i].hasFace==0)return;
    
	//Build shape or texture data.
	if (bMakeContours_) {
		if (printVectorInfo)printf("\n\ncharacter charID %d: \n", i );
		charOutlines[i] = makeContoursForCharacter(set.face);
		if (simplifyAmt_>0)charOutlines[i].simplify(simplifyAmt_);
		charOutlines[i].getTessellation();
	}
    
	if(bUseTexture_){
		if (bAntiAliased_ == true)
			FT_Render_Glyph(set.face->glyph, FT_RENDER_MODE_NORMAL);
		else
			FT_Render_Glyph(set.face->glyph, FT_RENDER_MODE_MONO);
        
		//------------------------------------------
		FT_Bitmap& bitmap = set.face->glyph->bitmap;
        
		ofPixels expandedData;
		// Allocate Memory For The Texture Data.
		expandedData.allocate(width, height, 2);
		//-------------------------------- clear data:
		expandedData.set(0,255); // every luminance pixel = 255
		expandedData.set(1,0);
        
		if (bAntiAliased_ == true) {
			ofPixels bitmapPixels;
			bitmapPixels.setFromExternalPixels(bitmap.buffer,bitmap.width,bitmap.rows,1);
			expandedData.setChannel(1,bitmapPixels);
		}else {
			//-----------------------------------
			// true type packs monochrome info in a
			// 1-bit format, hella funky
			// here we unpack it:
			unsigned char *src =  bitmap.buffer;
			for(int j=0; j<bitmap.rows; ++j) {
				unsigned char b=0;
				unsigned char *bptr =  src;
				for(int k=0; k<bitmap.width; ++k){
					expandedData[2*(k+j*width)] = 255;
                    
					if (k%8==0)
						b = (*bptr++);
                    
					expandedData[2*(k+j*width) + 1] = b&0x80 ? 255 : 0;
					b <<= 1;
				}
				src += bitmap.pitch;
			}
			//-----------------------------------
		}
        
		int longSide = border_ * 2;
		width > height ? longSide += width : longSide += height;
        
		int tmp = 1;
		while (longSide > tmp) {
			tmp <<= 1;
		}
		int w = tmp;
		int h = w;
        
		ofPixels atlasPixels;
		atlasPixels.allocate(w,h,2);
		atlasPixels.set(0,255);
		atlasPixels.set(1,0);
        
		cps[i].t2 = float(border_) / float(w);
		cps[i].v2 = float(border_) / float(h);
		cps[i].t1 = float(width + border_) / float(w);
		cps[i].v1 = float(height + border_) / float(h);
		expandedData.pasteInto(atlasPixels, border_, border_);
		
		int glFormat[] = {GL_LUMINANCE_ALPHA,GL_LUMINANCE_ALPHA};
		bool swizzle = false;
		
		#ifndef TARGET_OPENGLES
        if (ofIsGLProgrammableRenderer()) {
			glFormat[0] = GL_RG16;
			glFormat[1] = GL_RG;
			swizzle = true;
        } else {
            glFormat[0] = GL_LUMINANCE16_ALPHA16;
            glFormat[1] = GL_LUMINANCE_ALPHA;
        }
		#endif
        
		textures[i].allocate(atlasPixels.getWidth(), atlasPixels.getHeight(), glFormat[0], false);
        
		if (swizzle) {
			textures[i].setRGToRGBASwizzles( true );
		}
		
		if (bAntiAliased_ && set.fontsize>20) {
			textures[i].setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
		}else {
			textures[i].setTextureMinMagFilter(GL_NEAREST,GL_NEAREST);
		}
		
		textures[i].loadData(atlasPixels.getPixels(), atlasPixels.getWidth(), atlasPixels.getHeight(), glFormat[1]);
	}
}



//-----------------------------------------------------------
void ofxTrueTypeFontUL2::Impl::drawChar(int c, float x, float y) {
	if (c >= textures.size()) {
		//ofLog(OF_LOG_ERROR,"Error : char (%i) not allocated -- line %d in %s", (c + NUM_CHARACTER_TO_START), __LINE__,__FILE__);
		return;
	}
    
	///bind()/////////////////////////////////////////////////////////
    
	// we need transparency to draw text, but we don't know
	// if that is set up in outside of this function
	// we "pushAttrib", turn on alpha and "popAttrib"
	// http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/gl/pushattrib.html
    
	// **** note ****
	// I have read that pushAttrib() is slow, if used often,
	// maybe there is a faster way to do this?
	// ie, check if blending is enabled, etc...
	// glIsEnabled().... glGet()...
	// http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/gl/get.html
	// **************
	// (a) record the current "alpha state, blend func, etc"
#ifndef TARGET_OPENGLES
	glPushAttrib(GL_COLOR_BUFFER_BIT);
#else
	blend_enabled = glIsEnabled(GL_BLEND);
	texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
	glGetIntegerv( GL_BLEND_SRC, &blend_src );
	glGetIntegerv( GL_BLEND_DST, &blend_dst );
#endif
	// (b) enable our regular ALPHA blending!
    
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	textures[c].bind();
	stringQuads.clear();
    
    
	//drawChar()//////////////////////////////////////////////////////
    
	x=floor(x);
	y=floor(y);
    
	GLfloat	x1, y1, x2, y2;
	GLfloat t1, v1, t2, v2;
	t2 = cps[c].t2;
	v2 = cps[c].v2;
	t1 = cps[c].t1;
	v1 = cps[c].v1;
    
	x1 = cps[c].x1+x;
	y1 = cps[c].y1+y;
	x2 = cps[c].x2+x;
	y2 = cps[c].y2+y;
    
	int firstIndex = stringQuads.getVertices().size();
    
	stringQuads.addVertex(ofVec3f(x1,y1));
	stringQuads.addVertex(ofVec3f(x2,y1));
	stringQuads.addVertex(ofVec3f(x2,y2));
	stringQuads.addVertex(ofVec3f(x1,y2));
    
	stringQuads.addTexCoord(ofVec2f(t1,v1));
	stringQuads.addTexCoord(ofVec2f(t2,v1));
	stringQuads.addTexCoord(ofVec2f(t2,v2));
	stringQuads.addTexCoord(ofVec2f(t1,v2));
    
	stringQuads.addIndex(firstIndex);
	stringQuads.addIndex(firstIndex+1);
	stringQuads.addIndex(firstIndex+2);
	stringQuads.addIndex(firstIndex+2);
	stringQuads.addIndex(firstIndex+3);
	stringQuads.addIndex(firstIndex);
    
	//unbind()//////////////////////////////////////////////////////////
    
	stringQuads.drawFaces();
	textures[c].unbind();
    
#ifndef TARGET_OPENGLES
	glPopAttrib();
#else
	if (!blend_enabled)glDisable(GL_BLEND);
	if (!texture_2d_enabled)glDisable(GL_TEXTURE_2D);
	glBlendFunc( blend_src, blend_dst );
#endif
    
}
//-----------------------------------------------------------
void ofxTrueTypeFontUL2::Impl::drawCharAsShape(int c, float x, float y) {
	if (c >= charOutlines.size()) {
		//ofLog(OF_LOG_ERROR,"Error : char (%i) not allocated -- line %d in %s", (c + NUM_CHARACTER_TO_START), __LINE__,__FILE__);
		return;
	}
	ofPath & charRef = charOutlines[c];
	charRef.setFilled(ofGetStyle().bFill);
	if(strokeWidth>0.0)charRef.setStrokeWidth(strokeWidth);
	charRef.draw(x,y);
}

//--------------------------------------------------------
void ofxTrueTypeFontUL2::setGlobalDpi(int newDpi){
	ttfGlobalDpi = newDpi;
}
//--------------------------------------------------------

#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
#include <set>
static set<ofxTrueTypeFontUL2*> & all_fonts(){
    static set<ofxTrueTypeFontUL2*> *all_fonts = new set<ofxTrueTypeFontUL2*>;
    return *all_fonts;
}

void ofUnloadAllFontTextures(){
    set<ofxTrueTypeFontUL2*>::iterator it;
    for(it=all_fonts().begin();it!=all_fonts().end();it++){
        (*it)->unloadFont();
    }
}

void ofReloadAllFontTextures(){
    set<ofxTrueTypeFontUL2*>::iterator it;
    for(it=all_fonts().begin();it!=all_fonts().end();it++){
        (*it)->reloadFont();
    }
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ofxTrueTypeFontUL2
ofxTrueTypeFontUL2::ofxTrueTypeFontUL2() {
	initLibraries();
    
	mImpl = new Impl();
    
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
	all_fonts().insert(this);
#endif
    
	setTextDirection(UL2_TEXT_DIRECTION_LTR);
	setLetterSpacing(0.0f);
	setSpaceSize(1.0f);
	setWordWrap(true);
	useProportional(false);
	useVrt2Layout(false);
	setUseLayoutCache(true);
}

//------------------------------------------------------------------
ofxTrueTypeFontUL2::~ofxTrueTypeFontUL2() {
	if (mImpl->bLoadedOk_)unloadFont();
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IPHONE)
	all_fonts().erase(this);
#endif
	if (mImpl != NULL)delete mImpl;
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUL2::loadFont(string filename, float fontsize, bool _bAntiAliased, bool makeContours, float _simplifyAmt, int _dpi,bool useTexture,string scriptTagName) {
	return mImpl->implLoadFont(filename, fontsize, _bAntiAliased, makeContours, _simplifyAmt, _dpi,useTexture,scriptTagName);
}

bool ofxTrueTypeFontUL2::loadSubFont(string filename,float sizeRate, float baseLineRate,int unicodeRangeStart,int unicodeRangeEnd,string scriptTagName){
	return mImpl->implLoadSubFont(filename, sizeRate, baseLineRate,unicodeRangeStart,unicodeRangeEnd,scriptTagName);
}

void ofxTrueTypeFontUL2::reloadFont() {
	if(mImpl->faces.size()==0){
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::reloadFont - Error Font is not loaded.");
		return;
	}
	mImpl->implLoadFont(mImpl->faces[0].filename, mImpl->faces[0].fontsize, mImpl->bAntiAliased_, mImpl->bMakeContours_, mImpl->simplifyAmt_, mImpl->dpi_,mImpl->bUseTexture_, mImpl->faces[0].scriptTagName);
	for(int i=1 ; i< mImpl->faces.size(); i++){
		mImpl->implLoadSubFontVal(mImpl->faces[i].filename, mImpl->faces[i].fontsize, mImpl->faces[i].baseLine,mImpl->faces[i].unicodeRangeStart,mImpl->faces[i].unicodeRangeEnd,mImpl->faces[i].scriptTagName);
	}
}

void ofxTrueTypeFontUL2::unloadFont() {
	mImpl->implUnloadFont();
}

//-----------------------------------------------------------
bool ofxTrueTypeFontUL2::isLoaded() {
	return mImpl->bLoadedOk_;
}
bool ofxTrueTypeFontUL2::isAntiAliased() {
	return mImpl->bAntiAliased_;
}

//-----------------------------------------------------------
float ofxTrueTypeFontUL2::getFontSize() {
	return mImpl->baseFontSize_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::setLineHeight(float _newLineHeight) {
	mImpl->lineHeight_ = _newLineHeight;
}
float ofxTrueTypeFontUL2::getLineHeight() {
	return mImpl->lineHeight_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::setLetterSpacing(float _newletterSpacing) {
	mImpl->letterSpacing_ = _newletterSpacing;
	if(mImpl->letterSpacing_!=0.0f){
		addOTFeature("liga",0);
	}else{
		removeOTFeature("liga");
	}
}

float ofxTrueTypeFontUL2::getLetterSpacing() {
	return mImpl->letterSpacing_;
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::setSpaceSize(float _newspaceSize) {
	mImpl->spaceSize_ = _newspaceSize;
}
float ofxTrueTypeFontUL2::getSpaceSize() {
	return mImpl->spaceSize_;
}
//-----------------------------------------------------------
bool ofxTrueTypeFontUL2::getWordWrap(){
	return mImpl->bWordWrap;
}
void ofxTrueTypeFontUL2::setWordWrap(bool wordwrap){
	mImpl->bWordWrap=wordwrap;
}

void ofxTrueTypeFontUL2::addOTFeature(const char*feature_tag,unsigned int value,unsigned int  start,unsigned int end){
	mImpl->addOTFeature(feature_tag,value,start,end);
}
void ofxTrueTypeFontUL2::removeOTFeature(const char*feature_tag){
	mImpl->removeOTFeature(feature_tag);
}
void ofxTrueTypeFontUL2::clearOTFeatures(){
	mImpl->clearOTFeatures();
}
void ofxTrueTypeFontUL2::printOTFeatures(){
	mImpl->printOTFeatures();
}

void ofxTrueTypeFontUL2::setAlignByPixel(bool alignByPixel){
	mImpl->bAlignByPixel=alignByPixel;
}
bool ofxTrueTypeFontUL2::getAlignByPixel(){
	return mImpl->bAlignByPixel;
}

void  ofxTrueTypeFontUL2::setStrokeWidth(float width){
	mImpl->strokeWidth=width;
}
float  ofxTrueTypeFontUL2::getStrokeWidth(){
	return mImpl->strokeWidth;
}


//shrt cut feature

void ofxTrueTypeFontUL2::useProportional(bool bUseProportional){
	mImpl->bUseProportional=bUseProportional;
	removeOTFeature("vpal");
	removeOTFeature("palt");
	if(bUseProportional){
		if(mImpl->bWritingHorizontal)addOTFeature("palt",1);
		else addOTFeature("vpal",1);
	}
}
void ofxTrueTypeFontUL2::useVrt2Layout(bool bUseVrt2Layout){
	mImpl->bUseVrt2Layout=bUseVrt2Layout;
	removeOTFeature("vrt2");
	if(bUseVrt2Layout && !mImpl->bWritingHorizontal )addOTFeature("vrt2",1);
}
//-----------------------------------------------------------
void ofxTrueTypeFontUL2::setTextDirection(ul2_text_direction direction,ul2_text_direction subDirection ){
	if(direction==UL2_TEXT_DIRECTION_RTL){
		//Right to Left
		mImpl->m_direction = HB_DIRECTION_RTL;
		mImpl->m_subDirection = subDirection==UL2_TEXT_DIRECTION_BTT?HB_DIRECTION_BTT:HB_DIRECTION_TTB;
		mImpl->bWritingHorizontal=true;
	}else if(direction==UL2_TEXT_DIRECTION_TTB){
		//Top to Bottom
		mImpl->m_direction = HB_DIRECTION_TTB;
		mImpl->m_subDirection = subDirection==UL2_TEXT_DIRECTION_LTR?HB_DIRECTION_LTR:HB_DIRECTION_RTL;
		mImpl->bWritingHorizontal=false;
	}else if(direction==UL2_TEXT_DIRECTION_BTT){
		//Bottom to Top
		mImpl->m_direction = HB_DIRECTION_BTT;
		mImpl->m_subDirection = subDirection==UL2_TEXT_DIRECTION_RTL?HB_DIRECTION_RTL:HB_DIRECTION_LTR;
		mImpl->bWritingHorizontal=false;
	}else{
		//Left to Right
		mImpl->m_direction = HB_DIRECTION_LTR;
		mImpl->m_subDirection = subDirection==UL2_TEXT_DIRECTION_BTT?HB_DIRECTION_BTT:HB_DIRECTION_TTB;
		mImpl->bWritingHorizontal=true;
	}
	useProportional(mImpl->bUseProportional);
	useVrt2Layout(mImpl->bUseVrt2Layout);
	mImpl->hbPositionCache.clear();
}

//=====================================================================

void  ofxTrueTypeFontUL2::setUseLayoutCache(bool useLayoutCache){
	if(!useLayoutCache)clearCache();
	mImpl->bUseLayoutCache=useLayoutCache;
}
bool  ofxTrueTypeFontUL2::getUseLayoutCache(){
	return mImpl->bUseLayoutCache;
}

void ofxTrueTypeFontUL2::clearCache(bool all){
	if(all){
		mImpl->cps.clear();
		mImpl->textures.clear();
		mImpl->loadedChars.clear();
		mImpl->charOutlines.clear();
	}
	mImpl->hbPositionCache.clear();
}

//-----------------------------------------------------------
vector<ofRectangle> ofxTrueTypeFontUL2::getStringBoxes(wstring src, float x, float y,float width,float height,int textAlign){
	vector<ofRectangle> result;
	if (!mImpl->bLoadedOk_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::getCaracterBoundingBoxes - font not allocated");
		return result;
	}
	mImpl->commonLayouts(src,x,y,width, height, textAlign,UL2_GET_BOXES,result);
	return result;
}
vector<ofRectangle> ofxTrueTypeFontUL2::getStringBoxes(string src, float x, float y,float width,float height,int textAlign){
	return getStringBoxes(ul2_ttf_utils::convToWString(src), x, y,width, height, textAlign);
}

//-----------------------------------------------------------
ofRectangle ofxTrueTypeFontUL2::getStringBoundingBox(wstring src, float x, float y,float width,float height,int textAlign){
	ofRectangle rect(0,0,0,0);
	if (!mImpl->bLoadedOk_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::getStringBoundingBox - font not allocated");
		return rect;
	}
	mImpl->commonLayouts(src,x,y,width, height, textAlign,UL2_GET_BOUNDINGBOX,rect);
	return rect;
}
ofRectangle ofxTrueTypeFontUL2::getStringBoundingBox(string src, float x, float y,float width,float height,int textAlign){
	return getStringBoundingBox(ul2_ttf_utils::convToWString(src), x, y,width, height, textAlign);
}


//-----------------------------------------------------------
vector<ofPath> ofxTrueTypeFontUL2::getStringAsPoints(wstring src,float x, float y,float width,float height,int textAlign) {
	vector<ofPath> shapes;
	if (!mImpl->bLoadedOk_) {
		ofLog(OF_LOG_ERROR,"Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
		return shapes;
	}; 
	if (!mImpl->bMakeContours_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::drawStringAsShapes - Error : contours not created for this font - call loadFont with makeContours set to true");
		return shapes;
	}
	mImpl->commonLayouts(src,x,y,width, height, textAlign,UL2_GET_SHAPES,shapes);
	return shapes;
}
vector<ofPath> ofxTrueTypeFontUL2::getStringAsPoints(string src,float x, float y,float width,float height,int textAlign) {
	return getStringAsPoints(ul2_ttf_utils::convToWString(src),x,y,width, height, textAlign);
}


ofPath ofxTrueTypeFontUL2::getCharacterAsPoints(wstring src) {  
	ul2_ttf_utils::ustring character=ul2_ttf_utils::convertTTFwstring(src);
	return getStringAsPoints(src.substr(0,1))[0];
}
ofPath ofxTrueTypeFontUL2::getCharacterAsPoints(string character) {
	return getCharacterAsPoints(ul2_ttf_utils::convToWString(character));
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::drawString(wstring src, float x, float y,float width,float height,int textAlign) {
	if (!mImpl->bLoadedOk_){
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::drawString - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
		return;
	};
	if(!mImpl->bUseTexture_){
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::drawString - textures not created,  call loadFont with useTexture set to true");
		return;
	}
	bool result;
	mImpl->commonLayouts(src,x,y,width, height, textAlign,UL2_DRAW_TEXTURE,result);
}
void ofxTrueTypeFontUL2::drawString(string src, float x, float y,float width,float height,int textAlign) {
	return drawString(ul2_ttf_utils::convToWString(src), x, y,width, height, textAlign);
}

//-----------------------------------------------------------
void ofxTrueTypeFontUL2::drawStringAsShapes(wstring s, float x, float y,float width,float height,int textAlign) {
	if (!mImpl->bLoadedOk_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::drawStringAsShapes - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
		return;
	}
	if (!mImpl->bMakeContours_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::drawStringAsShapes - Error : contours not created for this font - call loadFont with makeContours set to true");
		return;
	}
	bool result;
	mImpl->commonLayouts(s,x,y,width, height, textAlign,UL2_RRAW_SHAPE,result);
}
void ofxTrueTypeFontUL2::drawStringAsShapes(string s, float x, float y,float width,float height,int textAlign) {
	return drawStringAsShapes(ul2_ttf_utils::convToWString(s), x, y,width, height, textAlign);
}

	//Ready for ofx3DFont
void ofxTrueTypeFontUL2::getLayoutData(vector<ofxFaceVec2>&facePosis,wstring s, float x, float y,float width,float height,int textAlign){
	if (!mImpl->bLoadedOk_) {
		ofLog(OF_LOG_ERROR,"ofxTrueTypeFontUL2::getLayoutData - Error : font not allocated -- line %d in %s", __LINE__,__FILE__);
		return;
	}
	mImpl->commonLayouts(s,x,y,width, height, textAlign,UL2_GET_LAYOUTS,facePosis);
}
void ofxTrueTypeFontUL2::getLayoutData(vector<ofxFaceVec2>&facePosis,string s, float x, float y,float width,float height,int textAlign){
	getLayoutData(facePosis,ul2_ttf_utils::convToWString(s), x, y,width, height, textAlign);
}
ofPath ofxTrueTypeFontUL2::getCountours(int index){
	return mImpl->charOutlines[index];
}

//==========================================================================================================================================
int ofxTrueTypeFontUL2::getLoadedCharactersCount() {
	return mImpl->cps.size();
}

//-------------------------------------------------------------
bool ofxTrueTypeFontUL2::initLibraries(){
	if(!librariesInitialized){
	    FT_Error err;
	    err = FT_Init_FreeType( &library );
        
	    if (err){
			ofLogError("ofTrueTypeFont") << "loadFont(): couldn't initialize Freetype lib: FT_Error " << err;
			return false;
		}
#ifdef TARGET_LINUX
		FcBool result = FcInit();
		if(!result){
			return false;
		}
#endif
#ifdef TARGET_WIN32
		initWindows();
#endif
		librariesInitialized = true;
	}
    return true;
}

void ofxTrueTypeFontUL2::finishLibraries(){
	if(librariesInitialized){
#ifdef TARGET_LINUX
		//FcFini();
#endif
		FT_Done_FreeType(library);
	}
}

