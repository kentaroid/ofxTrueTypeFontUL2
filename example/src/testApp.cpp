#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    
	ofBackground(255,255,255);
	ofNoFill();
    faceNavi.loadFont(OF_TTF_SANS ,15);  //latin
	  

	//Load three font-faces.//////////////////////////////////////////////////////////


    //Load japanse face.
    face.loadFont("Yumin Demibold",24,true,true,0.3f,0,true)|| // windows 8.1
		face.loadSubFont("YuMincho")|| // osx mavericks
		face.loadSubFont("Meiryo")|| // windows 7
		face.loadSubFont("Hiragino Mincho ProN W3"); // osx 
	
	//Load Latin face.   Fontsize = 120% , Baseline =-2% (to under)
	face.loadSubFont(OF_TTF_SERIF,1.2,-0.02);  //latin (OF_TTF_SERIF having arabic faces.)

	//Load arabic face.   UNICODE:0x0600-0x06FF
    face.loadSubFont("Traditional Arabic",1,-0.04,0x0600,0x06FF)|| // windows
		face.loadSubFont("Geeza Pro",1,-0.04,0x0600,0x06FF,"arab");  // osx (Geeza need to set scriptTag.)




	//Set options.//////////////////////////////////////////////////////////////////////
	//face.setLineHeight(face.getFontSize()*2);
    //face.setLetterSpacing(0.1);
	//face.setTextDirection(UL2_TEXT_DIRECTION_RTL,UL2_TEXT_DIRECTION_TTB);


    //enable other any OT feature tags.//////////////////////////////////////////////
    //http://partners.adobe.com/public/developer/opentype/index_tag3.html
    //face.addOTFeature("liga",1);
    
    //this made proportional fonts, these are OpenType features tag by "palt" or "vpal".
	//For the faces not having kerning pairs, such as Japanese fonts.
    face.useProportional(true);
    face.useVrt2Layout(true);
    

    //text////////////////////////////////////////////////////////////////////////////
	show.append(L"ofxTrueTypeFontUL2: Openframeworks タイポグラフィ\n");
	show.append(L"Wrote arabic: الأطر المفتوحة الطباعة غير موقعة طويل\n");
	show.append(L"penFrameworksのAPIは、常に進化しています。ぜひ、修正、追加、コメントをしてください。\n");
	show.append(L"The openFrameworks API is constantly evolving: any corrections, additions or comments are very welcome!");
	


    bitmapRendering=false;
    align=UL2_TEXT_ALIGN_INVALID;
    strAlign="No Alignment [0]";
    strDirection="Left to Right (TTB)";
}

//--------------------------------------------------------------
void testApp::update(){
	ofSetWindowTitle(ofToString(ofGetFrameRate()));
}

//--------------------------------------------------------------
void testApp::draw(){
    
	int w,h,x,y;
	x=50;
	y=50;
	w=mouseX-x;
	h=mouseY-y;
	
	//base line
	ofSetColor(255,127,255,255);
	ofLine(0,y,ofGetWidth(),y);
	ofLine(x,0,x,ofGetHeight());
	
	//cursor
	ofSetColor(127,127,255,255);
	ofLine(mouseX,0,mouseX,ofGetHeight());
	ofLine(0,mouseY,ofGetWidth(),mouseY);
    
	//draw
	if(bitmapRendering){
		ofSetColor(0,0,0,255);
		face.drawString(show,x,y,w,h,align);
	}else{
		ofSetColor(127,255,127,255);
		vector<ofRectangle> r = face.getStringBoxes(show,x,y,w,h,align);
		for(int i=0;i<r.size();i++)ofRect(r[i]);
		ofSetColor(255,127,127,255);
		face.drawStringAsShapes(show,x,y,w,h,align);
		ofSetColor(255,127,255,255);
		ofRect(face.getStringBoundingBox(show,x,y,w,h,align));
	}
    
    ofSetColor(0,0,0,255);
    faceNavi.drawString("Rendering mode [Space]: "+ofToString(bitmapRendering?"Bitmap":"Shapes"),50,ofGetHeight()-80);
    faceNavi.drawString("Wrap-mode [key w]: "+ ofToString(face.getWordWrap()?"Word-wrap":"Character-wrap"),50,ofGetHeight()-50);
    faceNavi.drawString("Alignment-mode [key e]: "+ ofToString(face.getAlignByPixel()?"Alignment by Pixel":"Optimized"),50,ofGetHeight()-20);
    faceNavi.drawString("Text-Alignment [key 0-9]: "+strAlign,ofGetWidth()*.5,ofGetHeight()-50);
    faceNavi.drawString("Text-Direction [key a,z,s,x,d,c,f,v]: "+strDirection,ofGetWidth()*.5,ofGetHeight()-20);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    
    switch(key){
        case ' ':
            bitmapRendering=!bitmapRendering;
            break;
        case 'w':
        case 'W':
            face.setWordWrap(!face.getWordWrap());
            break;
        case 'q':
        case 'Q':
            //face.setUseLayoutCache(!face.getUseLayoutCache());
            break;
        case 'e':
        case 'E':
            face.setAlignByPixel(!face.getAlignByPixel());
            break;
        case '0':
            align = UL2_TEXT_ALIGN_INVALID;
            strAlign="No Alignment [0]";
            break;
        case '1':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_LEFT;
            strAlign="bottom-left [1]";
            break;
        case '2':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_CENTER;
            strAlign="bottom-center [2]";
            break;
        case '3':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_RIGHT;
            strAlign="bottom-right [3]";
            break;
        case '4':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_LEFT;
            strAlign="middle-left [4]";
            break;
        case '5':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_CENTER;
            strAlign="middle-center [5]";
            break;
        case '6':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_RIGHT;
            strAlign="middle-right [6]";
            break;
        case '7':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_LEFT;
            strAlign="top-left [7]";
            break;
        case '8':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_CENTER;
            strAlign="top-center [8]";
            break;
        case '9':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_RIGHT;
            strAlign="top-right [9]";
            break;
		case 'a':
		case 'A':
			face.setTextDirection(UL2_TEXT_DIRECTION_LTR,UL2_TEXT_DIRECTION_TTB);
            strDirection="Left to Right (TTB)";
			break;
		case 's':
		case 'S':
			face.setTextDirection(UL2_TEXT_DIRECTION_RTL,UL2_TEXT_DIRECTION_TTB);
            strDirection="Right to Left (TTB)";
			break;
		case 'd':
		case 'D':
			face.setTextDirection(UL2_TEXT_DIRECTION_TTB,UL2_TEXT_DIRECTION_LTR);
            strDirection="Top to Bottom (LTR)";
			break;
		case 'f':
		case 'F':
			face.setTextDirection(UL2_TEXT_DIRECTION_BTT,UL2_TEXT_DIRECTION_LTR);
            strDirection="Bottom to Top (LTR)";
			break;
		case 'z':
		case 'Z':
			face.setTextDirection(UL2_TEXT_DIRECTION_LTR,UL2_TEXT_DIRECTION_BTT);
            strDirection="Left to Right (BTT)";
			break;
		case 'x':
		case 'X':
			face.setTextDirection(UL2_TEXT_DIRECTION_RTL,UL2_TEXT_DIRECTION_BTT);
            strDirection="Right to Left (BTT)";
			break;
		case 'c':
		case 'C':
			face.setTextDirection(UL2_TEXT_DIRECTION_TTB,UL2_TEXT_DIRECTION_RTL);
            strDirection="Top to Bottom (RTL)";
			break;
		case 'v':
		case 'V':
			face.setTextDirection(UL2_TEXT_DIRECTION_BTT,UL2_TEXT_DIRECTION_RTL);
            strDirection="Bottom to Top (RTL)";
			break;
            
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
    
}