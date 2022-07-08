#pragma once

#include "ofMain.h"
#include "ofxEasyCubemap.hpp"

// 카메라의 현재 위치 및 fov(시야각)값을 받는 구조체 타입 지정. (구조체 타입은 ts interface 랑 비슷한 개념이라고 생각하면 될 것 같음.)
struct CameraData {
    glm::vec3 pos;
    float fov;
};

struct SpotLight {
    glm::vec3 position; // 조명의 위치 (포인트 라이트는 디렉셔널 라이트와 달리, 조명 위치를 중심으로 멀어질수록 감쇄를 계산해줘야 하므로, 조명의 '방향'이 아닌 '위치'가 필요함!)
    glm::vec3 direction; // 스포트라이트 조명의 ConeDirection. 즉, 원뿔의 정 가운데 방향벡터 (이 벡터를 기준으로 각 프래그먼트의 toLight 벡터와 이루는 각도를 비교해서 조명의 영향을 받게 할 것인지 결정함.)
    glm::vec3 color; // 조명의 색상 (셰이더에서 계산된 노말벡터와 조명벡터의 내적값(= 디퓨즈 라이팅 값)과 곱해줄거임)
    float intensity; // 조명의 강도 (c++ 에서 조명의 색상과 곱해줘서 조명색의 밝기를 결정함.)
    float cutoff; // 스포트라이트 조명 원뿔의 최대 각도 범위 (cos 값으로 전달됨. 이 cutoff 각도를 벗어나는 영역은 조명의 영향을 0으로 만들어버림.)
};

class ofApp : public ofBaseApp{

    public:
        void setup();
        void update();
        void draw();

        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);
    
        // ofApp.cpp 에서 물 메쉬와 방패 메쉬를 그리는 함수를 분할해서 쪼개줄 것이므로, 각 함수의 메서드를 미리 선언해놓음.
        void drawWater(SpotLight& spotLight, glm::mat4& proj, glm::mat4& view);
        void drawShield(SpotLight& spotLight, glm::mat4& proj, glm::mat4& view);
        void drawSkybox(SpotLight& spotLight, glm::mat4& proj, glm::mat4& view); // ofApp.cpp 에서 큐브메쉬를 그리는 함수를 따로 추출하기 위해 선언한 메서드.

        
        ofMesh shieldMesh; // shield.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        ofMesh planeMesh; // plane.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        ofMesh cubeMesh; // cube.ply 모델링 파일을 로드해서 사용할 메쉬 객체 변수 선언
        
        ofImage waterNrm; // plane.ply 에 씌워줄 노말맵을 로드하기 위한 이미지 객체 변수 선언
        
        ofImage diffuseTex; // shield.ply 에 씌워줄 디퓨즈 맵을 로드하기 위한 이미지 객체 변수 선언
        ofImage nrmTex; // shield.ply 에 씌워줄 스펙 맵을 로드하기 위한 이미지 객체 변수 선언
        ofImage specTex; // shield.ply 에 씌워줄 노말맵을 로드하기 위한 이미지 객체 변수 선언
        
        ofShader blinnPhong; // shield.ply 에 Blinn-Phong 반사모델을 적용할 때 사용할 셰이더 객체 변수 선언
        ofShader waterShader; // plane.ply 에 물 셰이더를 적용할 때 사용할 셰이더 객체 변수 선언
        ofShader skyboxShader; // cube.ply 에 큐브맵 텍스쳐를 적용하기 위해 사용할 셰이더 객체 변수 선언

        CameraData cam; // 카메라 위치 및 fov(시야각)의 현재 상태값을 나타내는 구조체를 타입으로 갖는 멤버변수 cam 선언
    
        ofxEasyCubemap cubemap; // 오픈프레임웍스는 큐브맵을 지원하지 않으므로, 큐브맵 로드 및 유니폼 변수 전송에 필요한 커스텀 클래스 객체 변수 선언
};
