#pragma once

//--------------------------------------------------
//Unicode wchar utils
//thanx by ofxTrueTypeFontUC
//https://github.com/hironishihara
//https://github.com/hironishihara/ofxTrueTypeFontUC

#define NOT_END_OF_LINE_CHARS L"${£¥＄（［｢￡￥([｛〔〈《「『【〘〖〝‘“｟«—…‥〳〴〵"
#define NOT_START_OF_LINE_CHARS L"%}¢°‰′″℃゛゜ゝゞヽヾ！％），．：；？］｡｣､･ｧｨｩｪｫｬｭｮｯｰﾞﾟ￠,)]｝、〕〉》」』】〙〗〟’”｠»ーァィゥェォッャュョヮヵヶぁぃぅぇぉっゃゅょゎゕゖㇰㇱㇲㇳㇴㇵㇶㇸㇹㇷㇷ゚ㇺㇻㇼㇽㇾㇿ々〻‐゠–〜～　 ?!‼⁇⁈⁉・:;/。.\n"
#define BREAKABLE_CHARS NOT_START_OF_LINE_CHARS

#include "ofConstants.h"
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



//code by OF_0.8.0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool librariesInitialized = false;
static FT_Library library;
static bool printVectorInfo = false;

//--------------------------------------------------------
static ofTTFCharacter makeContoursForCharacter(FT_Face &face);
static ofTTFCharacter makeContoursForCharacter(FT_Face &face){

		//int num			= face->glyph->outline.n_points;
		int nContours	= face->glyph->outline.n_contours;
		int startPos	= 0;

		char * tags		= face->glyph->outline.tags;
		FT_Vector * vec = face->glyph->outline.points;

		ofTTFCharacter charOutlines;
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
							charOutlines.bezierTo(controlPoint1.x/64, controlPoint1.y/64, controlPoint2.x/64, controlPoint2.y/64, nextPoint.x/64, nextPoint.y/64);
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
								lastPoint = (conicPoint + lastConnic) / 2;

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
						charOutlines.quadBezierTo(lastPoint.x/64, lastPoint.y/64, conicPoint.x/64, conicPoint.y/64, nextPoint.x/64, nextPoint.y/64);

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
void initWindows();

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
 