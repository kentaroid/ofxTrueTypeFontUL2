#ofxTrueTypeFontUL2

This is the class which extended ofTrueTypeFont. 
The basic function is able to treat like ofTrueTypeFont.  
It is Including the **harfbuzz** that is an OpenType text shaping engine. 
It enables to use various functions important in typography, such as ligature and kerning pairs.

* Supported text direction and direction of line-break .
* Supported mixed font. (like a font-family)
* Supported text alignment.
* Supported box-layout and word-wrap.

---
##Install
It has been tested in openframeworks 0.8.0 but, vflip does not yet support.(vs12 and osx)  
Please add the source and library files to your project.

* **In Windows**  
It may be an earlier than 2.5 version of FreeType that included openframeworks. In that case, copy the files in `vs_patch_freetype2.5`  to openframeworks.


##Load Fonts
`bool loadFont(string filename, ... , bool useTexture=true,string scriptTagName="");`

* scriptTagName  
http://www.microsoft.com/typography/otspec/scripttags.htm  
Default script font is applied if you do not specify. No problem in many cases.  
ex) "arab"

##Load mixed font  
Use the following method to using a mixed font. 
`bool loadSubFont(string filename,float sizeRate=1.0f, float baseLineRate=0.0f ,  int unicodeRangeStart=0x0000,int unicodeRangeEnd=0x0000 ,string scriptTagName="" );`

* **sizeRate**  
Specify at a rate to base font size. 
* **baseLineRate**  
Use to rectify a baseline position. Specify at a rate to base font size.
* **unicodeRangeStart,unicodeRangeEnd**  
Specify the range of Unicode displayed with this font.  
ex on Arabic) `face.loadSubFont("Traditional Arabic",1,-0.04,0x0600,0x06FF);`  
When the Unicode range is omitted, The font read behind is displayed by priority.  

##Set text direction
Specify the text direction. This is not a text alignments.  
This feature is available languages that are written from right, such as Arabic, or vertical writing of Japanese.  
Specify the direction of a new line in **subDirection**.
`void  setTextDirection(ul2_text_direction direction,ul2_text_direction subDirection = UL2_TEXT_DIRECTION_INVALID);`
* UL2_TEXT_DIRECTION_LTR  ( Left to Right )
* UL2_TEXT_DIRECTION_RTL  ( Right to Left )
* UL2_TEXT_DIRECTION_TTB  ( Top to Bottom )
* UL2_TEXT_DIRECTION_BTT  ( Bottom to Top )


##Rendering text (with alignment)
Can text rendering by calling same functions as ofTrueTypeFont. These are layouted by baseline.  
In addition, box layout and the alignment is available.
`void drawString(string s, float x, float y,float width=0,float height=0,int textAlign=UL2_TEXT_ALIGN_INVALID);`
`drawStringAsShapes`,`getStringAsPoints`,`getStringBoundingBox`,`getStringBoxes` have a common layout functions.

* **width,height**  
If specified, the text to overflow will be a new line by "ward-wrap" or "character-warp".  
 (as specified `void setWardWrap()`)
* **alignment**  
In the area that specified in the height and width, alignment layout is enabled.  
Specify a combination of flag as `UL2_TEXT_ALIGN_RIGHT|UL2_TEXT_ALIGN_V_BOTTOM` such as bottom-right.  
Specify either performing alignment with based on baseline or strict pixels.  
 (as specified `void setAlignByPixel()`)


