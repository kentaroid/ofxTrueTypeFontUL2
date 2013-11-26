#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	ofBackground(255,255,255);
	ofNoFill();

	face.loadFont("A-OTF-ShueiShogoMStd-H.otf",24,true,true,0.3f,0,true);
	face.loadSubFont("trado.ttf",1.1);
	face.loadSubFont("Line-20.otf",1.1,-0.02);
	face.setLineHeight(face.getFontSize()*4);
	
    //face.setLetterSpacing(0.1);
	//face.setTextDirection(UL2_TEXT_DIRECTION_RTL);

	//show.append(L"LTAV film if SpaceキーでRenderingモード切り替え\n");
	show.append(L"Openframeworks タイポグラフィライブラリ ofxTrueTypeFontUL2 \n");
	//show.append(L"アラビア語表示：أهلاً وسهلاً بكم في قاموس ريوكاي");
	show.append(L" 混合文字：ご使用のコンピュータのJavaソフトウェア\n またはJava Runtime Environmentを、Java Runtime、Runtime Environment、Runtime、JRE、");
	bitmapRendering=false;
    
    align=UL2_TEXT_ALIGN_INVALID;
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
	glColor4f(1,0.9,0.9,1);
	ofLine(0,y,ofGetWidth(),y);
	ofLine(x,0,x,ofGetHeight());
	
	//cursor
	glColor4f(0.6,0.6,1,1);
	ofLine(mouseX,0,mouseX,ofGetHeight());
	ofLine(0,mouseY,ofGetWidth(),mouseY);

	//draw
	if(bitmapRendering){
		glColor4f(0.0,0.0,0.0,1);
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

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if(key == ' ')bitmapRendering=!bitmapRendering;
	//wrap mode changes to word or character wrap.
	if(key == 'w'||key=='W')face.setWordWrap(!face.getWordWrap());
	//Layout chace make rendering fast in draw functions. 
	if(key == 'q'||key=='Q')face.setUseLayoutCache(!face.getUseLayoutCache());
	if(key == 'e'||key=='E')face.setAlignByPixel(!face.getAlignByPixel());
    

    switch(key){
        case '0':
            align = UL2_TEXT_ALIGN_INVALID;
            break;
        case '1':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_LEFT;
            break;
        case '2':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_CENTER;
             break;
        case '3':
            align = UL2_TEXT_ALIGN_V_BOTTOM|UL2_TEXT_ALIGN_RIGHT;
             break;
        case '4':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_LEFT;
             break;
        case '5':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_CENTER;
             break;
        case '6':
            align = UL2_TEXT_ALIGN_V_MIDDLE|UL2_TEXT_ALIGN_RIGHT;
             break;
        case '7':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_LEFT;
             break;
        case '8':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_CENTER;
             break;
        case '9':
            align = UL2_TEXT_ALIGN_V_TOP|UL2_TEXT_ALIGN_RIGHT;
             break;
		case 'a':
		case 'A':
			face.setTextDirection(UL2_TEXT_DIRECTION_LTR,UL2_TEXT_DIRECTION_TTB);
			break;
		case 's':
		case 'S':
			face.setTextDirection(UL2_TEXT_DIRECTION_RTL,UL2_TEXT_DIRECTION_TTB);
			break;
		case 'd':
		case 'D':
			face.setTextDirection(UL2_TEXT_DIRECTION_TTB,UL2_TEXT_DIRECTION_LTR);
			break;
		case 'f':
		case 'F':
			face.setTextDirection(UL2_TEXT_DIRECTION_BTT,UL2_TEXT_DIRECTION_LTR);
			break;
		case 'z':
		case 'Z':
			face.setTextDirection(UL2_TEXT_DIRECTION_LTR,UL2_TEXT_DIRECTION_BTT);
			break;
		case 'x':
		case 'X':
			face.setTextDirection(UL2_TEXT_DIRECTION_RTL,UL2_TEXT_DIRECTION_BTT);
			break;
		case 'c':
		case 'C':
			face.setTextDirection(UL2_TEXT_DIRECTION_TTB,UL2_TEXT_DIRECTION_RTL);
			break;
		case 'v':
		case 'V':
			face.setTextDirection(UL2_TEXT_DIRECTION_BTT,UL2_TEXT_DIRECTION_RTL);
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