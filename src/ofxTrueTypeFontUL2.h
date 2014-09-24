#pragma once
#include "ofTrueTypeFont.h"

typedef enum {
	UL2_TEXT_DIRECTION_INVALID = 0 ,
	UL2_TEXT_DIRECTION_LTR = 1 ,
	UL2_TEXT_DIRECTION_RTL = 2,
	UL2_TEXT_DIRECTION_TTB = 4,
	UL2_TEXT_DIRECTION_BTT = 8
} ul2_text_direction;

typedef enum {
	UL2_TEXT_ALIGN_INVALID = 0 ,
	UL2_TEXT_ALIGN_LEFT    = 1 ,
	UL2_TEXT_ALIGN_CENTER  = 2 ,
	UL2_TEXT_ALIGN_RIGHT   = 4,
	UL2_TEXT_ALIGN_V_TOP    = 8,
	UL2_TEXT_ALIGN_V_MIDDLE = 16,
	UL2_TEXT_ALIGN_V_BOTTOM = 32
} ul2_text_align;

typedef struct {
	float x;
	float y;
	int faceIndex;
}ofxFaceVec2;

class ofxTrueTypeFontUL2 {
public:
	ofxTrueTypeFontUL2();
	virtual ~ofxTrueTypeFontUL2();
    
	//set the default dpi for all typefaces.
	static void setGlobalDpi(int newDpi);
    
	//-- default (without dpi), anti aliased, 96 dpi:
	bool loadFont(string filename,float fontsize, bool bAntiAliased=true, bool makeContours=false, float simplifyAmt=0.3, int dpi=0, bool useTexture=true,string scriptTagName="");
	//add sub font ,What was read afterwards is used strongly. (Or set unicode range correctly . )
	bool loadSubFont(string filename,float sizeRate=1.0f, float baseLineRate=0.0f ,int unicodeRangeStart=0x0000,int unicodeRangeEnd=0x0000 ,string scriptTagName="" );
    
	void reloadFont();
	void unloadFont();
    
    
	void drawString(wstring s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	void drawString(string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	
	void drawStringAsShapes(wstring s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	void drawStringAsShapes(string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	
	vector<ofPath> getStringAsPoints(wstring s,float x=0, float y=0,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	vector<ofPath> getStringAsPoints(string s,float x=0, float y=0,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
    
	ofRectangle getStringBoundingBox(wstring s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	ofRectangle getStringBoundingBox(string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	
	vector<ofRectangle> getStringBoxes(wstring s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	vector<ofRectangle> getStringBoxes(string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
    
	ofPath getCharacterAsPoints(wstring character);
	ofPath getCharacterAsPoints(string character);
	
	//Ready for ofx3DFont
	void getLayoutData(vector<ofxFaceVec2>&facePosis,string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	void getLayoutData(vector<ofxFaceVec2>&facePosis,wstring s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);
	ofPath getCountours(int index);

	bool  isLoaded();
	bool  isAntiAliased();
	int	  getLoadedCharactersCount();
    
	float getFontSize();
    
	float getLineHeight();
	void  setLineHeight(float height);
    
	float getLetterSpacing();
	void  setLetterSpacing(float spacing);
    
	float getSpaceSize();
	void  setSpaceSize(float size);
    
	void  setTextDirection(ul2_text_direction direction,ul2_text_direction subDirection = UL2_TEXT_DIRECTION_INVALID);
    
	bool getWordWrap();
	void setWordWrap(bool useWordwrap);
    

	//Set opentype features.
	void addOTFeature(const char*feature_tag,unsigned int value,unsigned int  start=0,unsigned int end=static_cast<unsigned>(-1));
	void removeOTFeature(const char*feature_tag);
	void clearOTFeatures();
	void printOTFeatures();
	void useProportional(bool bUseProportional);
	void useVrt2Layout(bool bUseVrt2Layout);
    
	//use caches (when drawing in fastloops)
	void setUseLayoutCache(bool useLayoutCache);
	bool getUseLayoutCache();
    void clearCache(bool all=false);

	//alignment option.
	void setAlignByPixel(bool alignByPixel);
	bool getAlignByPixel();
    
	//for drawStringAsShapes
	void setStrokeWidth(float width);
	float getStrokeWidth();
	
    
private:
#if defined(TARGET_ANDROID) || defined(TARGET_OF_IOS)
	friend void ofUnloadAllFontTextures();
	friend void ofReloadAllFontTextures();
#endif
	class Impl;
	Impl *mImpl;
	// disallow copy and assign
	ofxTrueTypeFontUL2(const ofxTrueTypeFontUL2 &);
	void operator=(const ofxTrueTypeFontUL2 &);
    
	static bool	initLibraries();
	static void finishLibraries();
    
};
