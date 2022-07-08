#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    // 아래 5줄은 초기의 main() 함수에서 원하는 버전의 OpenGL 을 사용하기 위해 수정해줘야 하는 부분들
    ofGLWindowSettings glSettings;
    glSettings.setSize(1024, 768);
    glSettings.windowMode = OF_WINDOW;
    glSettings.setGLVersion(4, 1);
    ofCreateWindow(glSettings); // 설정이 변경된 ofGLWindowSettings 객체를 ofCreateWindow() 함수에 전달해주면 실행창(윈도우)를 열어줌.
            
    // ofApp 객체 실행
    ofRunApp(new ofApp());
}
