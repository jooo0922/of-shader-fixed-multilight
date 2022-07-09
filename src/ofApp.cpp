#include "ofApp.h"
#include <vector> // calcTangents() 함수에서 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용하기 위해 해당 클래스를 include 시킴.

// 탄젠트 벡터 계산 후, 메쉬의 버텍스 컬러 데이터에 임시로 저장해두는 함수
void calcTangents(ofMesh& mesh) { // mesh 함수인자는 'ofMesh' 타입을 참조한다는 뜻. (diffuse-lighting 레포지토리 참조자 관련 설명 참고)
    using namespace glm;
    std::vector<vec4> tangents; // c++ 에 정의된 표준 라이브러리 사용 시, std:: 접두어를 항상 붙여주도록 함. 여기서는 동적 배열인 std::vector 컨테이너 클래스 템플릿을 사용함.
    tangents.resize(mesh.getNumVertices()); // 버텍스 개수만큼 탄젠트 벡터를 담는 동적 배열의 길이 생성
    
    uint indexCount = mesh.getNumIndices(); // 버텍스의 인덱스 개수만큼 인덱스 카운트 개수 지정
    
    const vec3* vertices = mesh.getVerticesPointer();
    const vec2* uvs = mesh.getTexCoordsPointer();
    const uint* indices = mesh.getIndexPointer();
    
    for (uint i = 0; i < indexCount - 2; i += 3) {
        // 3개의 인접한 버텍스들 -> 즉, 하나의 삼각형 안의 3개의 버텍스들의 위치데이터와 uv데이터를 각각 구함
        // 왜 삼각형일까? 삼각형만이 온전한 평면을 구성하고, 같은 평면안에 존재한다면 탄젠트 공간 내의 탄젠트 벡터도 동일한 걸 공유한다고 가정하기 때문인 것 같음.
        // 왜냐, 같은 평면 안에 존재하는 버텍스라면, 그 버텍스들은 동일한 노말벡터를 공유하기 때문에, 탄젠트 벡터도 동일하다고 봄.
        const vec3& v0 = vertices[indices[i]];
        const vec3& v1 = vertices[indices[i + 1]];
        const vec3& v2 = vertices[indices[i + 2]];
        const vec2& uv0 = uvs[indices[i]];
        const vec2& uv1 = uvs[indices[i + 1]];
        const vec2& uv2 = uvs[indices[i + 2]];
        
        // v0 버텍스에 연결된 2개의 선분(벡터)을 구함 -> 3차원 공간 상의 선분(벡터)
        vec3 edge1 = v1 - v0; // 선분(벡터)1
        vec3 edge2 = v2 - v0; // 선분(벡터)2
        // v0 버텍스의 uv좌표에 연결된 uv좌표계 상의 2개의 선분(벡터)도 구함 (아마 distance UV 의 줄임말인 듯.) -> 2차원 공간(uv좌표계) 상의 선분(벡터)
        vec2 dUV1 = uv1 - uv0; // uv좌표계 선분1
        vec2 dUV2 = uv2 - uv0; // uv좌표계 선분2
        
        // 위에서 구한 두 벡터를 기저 벡터로 삼아(?) 탄젠트 벡터를 구하는 과정같음.. (아직 자세히는 모르겠음ㅜ)
        float f = 1.0f / (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
        
        vec4 tan;
        tan.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
        tan.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
        tan.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);
        tan.w = 0;
        tan = normalize(tan); // 계산한 탄젠트 벡터를 정규화하여 길이를 1로 맞춤.
        
        // 서로 인접하는 3개의 버텍스, 즉 한 삼각형 안의 버텍스들은 모두 동일한 탄젠트 벡터를 갖도록
        // tangents 동적배열에 인접한 3개의 인덱스에 동일한 탄젠트 벡터를 넣어줌..
        tangents[indices[i]] += (tan);
        tangents[indices[i + 1]] += (tan);
        tangents[indices[i + 2]] += (tan);
    }
    
    int numColors = mesh.getNumColors(); // 오픈프레임웍스가 버텍스 탄젠트 데이터를 지원하지 않아서 버텍스 컬러데이터에 대신 넣어주려는 것.

    for (int i = 0; i < tangents.size(); ++i) {
        vec3 t = normalize(tangents[i]); // 위에 반복문에서 저장해 둔 각 인덱스의 탄젠트 벡터를 가져온 뒤, vec3에 할당함으로써 각 탄젠트벡터의 x, y, z 컴포넌트만 가져옴.
        
        // 탄젠트벡터 데이터가 컬러데이터와 개수가 안맞을 수 있기 때문에,
        // i가 전체 컬러데이터 수보다 적으면 기존 i번쩨 컬러데이터를 setColor() 로 탄젠트 벡터 데이터로 덮어쓰고,
        // i가 전체 컬러데이터 수보다 크다면, addColor() 로 탄젠트 벡터 데이터를 전달해서 컬러데이터를 새로 추가함.
        if (i >= numColors) {
            mesh.addColor(ofFloatColor(t.x, t.y, t.z, 0.0));
        } else {
            mesh.setColor(i, ofFloatColor(t.x, t.y, t.z, 0.0));
        }
    }
}

// 조명계산 최적화를 위해, 쉐이더에서 반복계산하지 않도록, c++ 에서 한번만 계산해줘도 되는 작업들을 수행하는 보조함수들
glm::vec3 getLightDirection(DirectionalLight& l) {
    // 조명벡터 direction에 -1을 곱해서 조명벡터의 방향을 뒤집어주고, 셰이더에서 내적계산을 해주기 위해 길이를 1로 정규화해서 맞춰줌.
    return glm::normalize(l.direction * -1.0f);
}

glm::vec3 getLightColor(DirectionalLight& l) {
    // 디렉셔널라이트 구조체에서 vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

glm::vec3 getLightColor(PointLight& l) {
    // 포인트라이트 구조체에서 vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

glm::vec3 getLightColor(SpotLight& l) {
    // 스포트라이트 구조체에서 vec3 값인 조명색상에 float 값인 조명강도를 스칼라배로 곱해줘서 조명색상의 밝기를 지정함.
    return l.color * l.intensity;
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofDisableArbTex(); // 스크린 픽셀 좌표를 사용하는 텍스쳐 관련 오픈프레임웍스 레거시 지원 설정 비활성화. (uv좌표계랑 다르니까!)
    ofEnableDepthTest(); // 깊이테스트를 활성화하여 z버퍼에 저장해서 각 요소에서 카메라와의 거리를 기준으로 앞뒤를 구분하여 렌더링할 수 있도록 함.
        
    // 이번에는 ofApp 맨 처음 설정에서 shieldMesh 를 바라보기 적당한 카메라 위치와 시야각을 지정함.
    cam.pos = glm::vec3(0, 0.75f, 1.0f); // 카메라 위치는 z축으로 1.0만큼 안쪽으로 들어가게 하고, 조명 연산 결과를 확인하기 위해 y축으로도 살짝 올려줌
    cam.fov = glm::radians(90.0f); // 원근 프러스텀의 시야각은 일반 PC 게임에서는 90도 전후의 값을 사용함. -> 라디안 각도로 변환하는 glm 내장함수 radians() 를 사용함.
        
    planeMesh.load("plane.ply"); // planeMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(planeMesh); // plane 메쉬의 버텍스들을 이용해서 각 버텍스의 탄젠트 벡터를 구한 뒤 버텍스 컬러데이터 자리에 저장하는 함수 실행
        
    shieldMesh.load("shield.ply"); // shieldMesh 메쉬로 사용할 모델링 파일 로드
    calcTangents(shieldMesh); // shield 메쉬에 탄젠트 벡터를 구한 뒤 버텍스 컬러 자리에 저장하는 함수 실행
    
    cubeMesh.load("cube.ply"); // cubeMesh 메쉬로 사용할 모델링 파일 로드

    waterShader.load("water.vert", "multiLightWater.frag"); // planeMesh 에 노말맵을 활용한 물셰이더를 적용하기 위한 셰이더 파일 로드
    blinnPhong.load("mesh.vert", "multiLight.frag"); // shieldMesh 에 (노말맵)텍스쳐를 활용한 Blinn-phong 반사모델을 적용하기 위한 셰이더 파일 로드
    skyboxShader.load("skybox.vert", "skybox.frag"); // cubeMesh 에 큐브맵 텍스쳐를 적용한 셰이더를 적용하기 위한 셰이더 파일 로드
        
    waterNrm.load("water_nrm.png"); // planeMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
    waterNrm.getTexture().setTextureWrap(GL_REPEAT, GL_REPEAT); // 프래그먼트 셰이더에서 노말맵 텍스쳐를 타일링하여 샘플링할 것이므로, 노말맵의 랩 모드를 반복으로 지정함.
        
    diffuseTex.load("shield_diffuse.png"); // shieldMesh 의 조명계산에서 디퓨즈 라이팅 계산에 사용할 텍스쳐 로드
    specTex.load("shield_spec.png"); // shieldMesh 의 조명계산에서 스펙큘러 라이팅 계산에 사용할 텍스쳐 로드
    nrmTex.load("shield_normal.png"); // shieldMesh 의 조명계산에서 노말맵으로 사용할 텍스쳐 로드
    
    // 큐브맵 텍스쳐를 로드함.
    cubemap.load("night_front.jpg", "night_back.jpg",
            "night_right.jpg", "night_left.jpg",
            "night_top.jpg", "night_bottom.jpg");
}

//--------------------------------------------------------------
void ofApp::update(){

}

// waterMesh 의 각종 변환행렬을 계산한 뒤, 유니폼 변수들을 전송해주면서 드로우콜을 호출하는 함수
void ofApp::drawWater(glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    static float t = 0.0f; // static 을 특정 함수 내에서 사용하는 것을 '정적 지역 변수'라고 하며, 이 할당문은 drawWater() 함수 최초 호출 시 1번만 실행됨.
    t += ofGetLastFrameTime(); // 이전 프레임과 현재 프레임의 시간 간격인 '델타타임'을 리턴받는 함수를 호출해서 유니폼 변수로 전송할 시간값 t에 매 프레임마다 더해줌.
    
    // waterMesh 의 모델행렬 계산 (회전행렬 및 크기행렬만 적용)
    vec3 right = vec3(1, 0, 0); // waterMesh 모델의 회전행렬을 계산할 시, x축 방향으로만 회전할 수 있도록 회전축 벡터를 구해놓음.
    mat4 rotation = rotate(radians(-90.0f), right); // waterMesh 회전행렬은 x축 기준으로 cAngle(-90도) 회전시킴.
    mat4 model = rotation * scale(vec3(5.0, 4.0, 4.0)); // 열 우선 행렬이므로, 원하는 행렬 곱셈과 반대순서인 회전행렬 * 크기행렬 순으로 곱해줌
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    /**
         모델의 버텍스가 갖고있는 기본 노말벡터는 오브젝트공간을 기준으로 되어있음.
         그러나, 조명계산을 하려면 이러한 노말벡터를 월드공간으로 변환해야 함.
                 
         그럼 노말벡터도 그냥 모델행렬인 mat4 model 을 버텍스 셰이더에서 곱해주면 되는거 아닌가?
         이렇게 할 수도 있지만, 만약 모델행렬에 '비일률적 크기행렬' (예를들어, (0.5, 1.0, 1.0) 이런거)
         가 적용되어 있다면, 특정 축마다 scale 이 다르게 늘어나는 과정에서 노말벡터의 방향이 휘어버리게 됨. -> p.190 참고
                 
         이걸 똑바르게 세워주려면, '노멀행렬' 이라는 새로운 행렬이 필요함.
         노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 정의할 수 있음.
                 
         역행렬, 전치행렬, 상단 3*3 행렬에 대한 각각의 개념은 위키백과, 구글링, 북마크한거 참고...
                 
         어쨋든 위의 정의에 따라 아래와 같이 노말행렬을 구하고, 버텍스 셰이더로 쏴주면 됨.
    */
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    
    ofShader& shd = waterShader; // 참조자 shd 는 ofShader 타입의 멤버변수 waterShader 를 참조하도록 함.
    
    // shd(waterShader 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("normTex", waterNrm, 0); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 1); // 환경맵 반사를 적용하기 위해 사용할 큐브맵 텍스쳐 유니폼 변수로 전송
    shd.setUniform1f("time", t); // uv 스크롤링에 사용할 시간값 유니폼 변수로 전송
    
    // 이제 각 조명유형별 구조체배열마다 별도로 유니폼 변수에 데이터를 전송해줘야 함.
    // 참고로, 유니폼 변수 이름의 경우, 쉐이더에서도 헤더파일에서 선언한 것과 유사하게 각 조명별 구조체배열을 uniform 변수로 정의하고 있으므로,
    // ofApp.cpp 에서 접근하고 있는 것과 동일한 방식으로 데이터를 전달하고자 하는 유니폼 변수명을 지정해주면 됨!
    shd.setUniform3f("directionalLights[0].direction", getLightDirection(dirLights[0]));
    shd.setUniform3f("directionalLights[0].color", getLightColor(dirLights[0]));
    
    shd.setUniform3f("pointLights[0].position", pointLights[0].position);
    shd.setUniform3f("pointLights[0].color", getLightColor(pointLights[0]));
    shd.setUniform1f("pointLights[0].radius", pointLights[0].radius);
    
    shd.setUniform3f("pointLights[1].position", pointLights[1].position);
    shd.setUniform3f("pointLights[1].color", getLightColor(pointLights[1]));
    shd.setUniform1f("pointLights[1].radius", pointLights[1].radius);
    
    shd.setUniform3f("spotLights[0].position", spotLights[0].position);
    shd.setUniform3f("spotLights[0].direction", spotLights[0].direction);
    shd.setUniform3f("spotLights[0].color", getLightColor(spotLights[0]));
    shd.setUniform1f("spotLights[0].cutoff", spotLights[0].cutoff);
    
    shd.setUniform3f("spotLights[1].position", spotLights[1].position);
    shd.setUniform3f("spotLights[1].direction", spotLights[1].direction);
    shd.setUniform3f("spotLights[1].color", getLightColor(spotLights[1]));
    shd.setUniform1f("spotLights[1].cutoff", spotLights[1].cutoff);
    
    shd.setUniform3f("ambientCol", glm::vec3(0.0, 0.0, 0.0)); // 환경광으로 사용할 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    planeMesh.draw(); // planeMesh(waterMesh) 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd(waterShader) 사용 중단
}

void ofApp::drawSkybox(glm::mat4& proj, glm::mat4& view) {
    using namespace glm; // 이제부터 이 함수블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
    
    // cubeMesh 의 모델행렬 계산 (이동행렬만 적용)
    mat4 model = translate(cam.pos); // 큐브맵의 위치(즉, 큐브맵의 오브젝트공간 원점)를 카메라 위치와 일치시킴으로써, 큐브맵 안쪽 중앙에 카메라가 위치하도록 함 -> 큐브맵을 skybox 로 만들기 위함!
    
    // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat4 mvp = proj * view * model; // 열 우선 행렬이라 원래의 곱셈 순서인 '모델 -> 뷰 -> 투영'의 반대 순서로 곱해줘야 함.
    
    ofShader& shd = skyboxShader; // 참조자 shd 는 ofShader 타입의 멤버변수 skyboxShader 를 참조하도록 함.
    
    glDepthFunc(GL_LEQUAL); // 스카이박스를 그리기 전 깊이비교모드를 변경함 (깊이비교모드 관련 필기 하단 참고) Less Equal 의 줄임말. 즉, >= (보다 작거나 같음. 이하)을 의미
    
    // shd(skyboxShader 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 0); // 커스텀 큐브맵 클래스로 셰이더의 유니폼 변수에 큐브맵 텍스쳐를 전송할 경우, 명시적으로 getTexture() 를 호출해야 함.
    
    cubeMesh.draw(); // cubeMesh 메쉬 드로우콜 호출하여 그림.
    
    shd.end();
    // shd(cubemapShader) 사용 중단
    
    glDepthFunc(GL_LESS); // 스카이박스를 다 그린 뒤 깊이비교모드를 원래대로 원상복구함. (깊이비교모드 관련 필기 하단 참고) Less 의 줄임말. 즉, > (보다 작음. 미만)을 의미
}

void ofApp::drawShield(glm::mat4& proj, glm::mat4& view) {
    using namespace glm;
    
    mat4 model = translate(vec3(0.0, 0.75, 0.0f)); // shieldMesh 의 모델행렬 계산 (이동행렬만 적용)
    mat4 mvp = proj * view * model; // 최적화를 위해 c++ 단에서 투영 * 뷰 * 모델행렬을 한꺼번에 곱해서 버텍스 셰이더에 전송함.
    mat3 normalMatrix = mat3(transpose(inverse(model))); // 노말행렬은 '모델행렬의 상단 3*3 역행렬의 전치행렬' 로 계산함.
    
    ofShader& shd = blinnPhong; // 참조자 shd 는 ofShader 타입의 멤버변수 blinnPhong 셰이더 객체를 참조하도록 함.
    
    // shd(blinnPhong 를 참조) 를 바인딩하여 사용 시작
    shd.begin();
    
    shd.setUniformMatrix4f("mvp", mvp); // 위에서 한꺼번에 합쳐준 mvp 행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix4f("model", model); // 버텍스 좌표를 월드좌표로 변환하기 위해 모델행렬만 따로 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniformMatrix3f("normalMatrix", normalMatrix); // 노말행렬을 버텍스 셰이더 유니폼 변수로 전송
    shd.setUniform3f("meshSpecCol", glm::vec3(1, 1, 1)); // 스펙큘러 색상을 흰색으로 지정하여 유니폼 변수로 전송
    shd.setUniformTexture("diffuseTex", diffuseTex, 0); // 디퓨즈 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("specTex", specTex, 1); // 스펙큘러 라이팅 계산에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("nrmTex", nrmTex, 2); // 노말 매핑에 사용할 텍스쳐 유니폼 변수로 전송
    shd.setUniformTexture("envMap", cubemap.getTexture(), 3); // 환경맵 반사를 적용하기 위해 사용할 큐브맵 텍스쳐 유니폼 변수로 전송
    
    // 이제 각 조명유형별 구조체배열마다 별도로 유니폼 변수에 데이터를 전송해줘야 함.
    // 참고로, 유니폼 변수 이름의 경우, 쉐이더에서도 헤더파일에서 선언한 것과 유사하게 각 조명별 구조체배열을 uniform 변수로 정의하고 있으므로,
    // ofApp.cpp 에서 접근하고 있는 것과 동일한 방식으로 데이터를 전달하고자 하는 유니폼 변수명을 지정해주면 됨!
    shd.setUniform3f("directionalLights[0].direction", getLightDirection(dirLights[0]));
    shd.setUniform3f("directionalLights[0].color", getLightColor(dirLights[0]));
    
    shd.setUniform3f("pointLights[0].position", pointLights[0].position);
    shd.setUniform3f("pointLights[0].color", getLightColor(pointLights[0]));
    shd.setUniform1f("pointLights[0].radius", pointLights[0].radius);
    
    shd.setUniform3f("pointLights[1].position", pointLights[1].position);
    shd.setUniform3f("pointLights[1].color", getLightColor(pointLights[1]));
    shd.setUniform1f("pointLights[1].radius", pointLights[1].radius);
    
    shd.setUniform3f("spotLights[0].position", spotLights[0].position);
    shd.setUniform3f("spotLights[0].direction", spotLights[0].direction);
    shd.setUniform3f("spotLights[0].color", getLightColor(spotLights[0]));
    shd.setUniform1f("spotLights[0].cutoff", spotLights[0].cutoff);
    
    shd.setUniform3f("spotLights[1].position", spotLights[1].position);
    shd.setUniform3f("spotLights[1].direction", spotLights[1].direction);
    shd.setUniform3f("spotLights[1].color", getLightColor(spotLights[1]));
    shd.setUniform1f("spotLights[1].cutoff", spotLights[1].cutoff);
    
    shd.setUniform3f("ambientCol", glm::vec3(0.1, 0.1, 0.1)); // 배경색과 동일한 앰비언트 라이트 색상값을 유니폼 변수로 전송.
    shd.setUniform3f("cameraPos", cam.pos); // 프래그먼트 셰이더에서 뷰 벡터를 계산하기 위해 카메라 좌표(카메라 월드좌표)를 프래그먼트 셰이더 유니폼 변수로 전송
    
    shieldMesh.draw(); // shieldMesh 메쉬 드로우콜 호출하여 그려줌.
    
    shd.end();
    // shd(blinnPhong) 사용 중단
}

//--------------------------------------------------------------
void ofApp::draw(){
    using namespace glm; // 이제부터 현재 블록 내에서 glm 라이브러리에서 꺼내 쓸 함수 및 객체들은 'glm::' 을 생략해서 사용해도 됨.
    
    // sin 함수로 포인트라이트 조명 위치를 매 프레임마다 -1 ~ 1 사이로 왕복하기 위해 sin 함수에 매 프레임마다 넣어줄 값을 계산함.
    static float t = 0.0f; // static 을 특정 함수 내에서 사용하는 것을 '정적 지역 변수'라고 하며, 이 할당문은 drawWater() 함수 최초 호출 시 1번만 실행됨.
    t += ofGetLastFrameTime(); // 이전 프레임과 현재 프레임의 시간 간격인 '델타타임'을 리턴받는 함수를 호출해서 sin함수의 인자로 사용할 시간값 t에 매 프레임마다 더해줌.
    
    // 헤더파일에서 선언한 각 조명 유형별 구조체배열에 조명데이터를 할당해 줌.
    // 1개의 디렉셔널라이트 구조체배열에 데이터 할당
    dirLights[0].color = vec3(1, 1, 0); // 조명색상 노랑색 할당
    dirLights[0].intensity = 1.0f;
    dirLights[0].direction = vec3(1, -1, -1);
    
    // 2개의 포인트라이트 구조체배열에 데이터 할당
    pointLights[0].color = vec3(1, 0, 0); // 조명색상 빨간색 할당
    pointLights[0].radius = 1.0f;
    pointLights[0].position = vec3(-0.5, 0.5, 0.25);
    pointLights[0].intensity = 1.0;
    
    pointLights[1].color = vec3(0, 1, 0); // 조명색상 초록색 할당
    pointLights[1].radius = 1.0f;
    pointLights[1].position = vec3(0.5, 0.5, 0.25);
    pointLights[1].intensity = 1.0;
    
    // 2개의 스포트라이트 구조체배열에 데이터 할당
    spotLights[0].color = vec3(0, 0, 1); // 조명색상 파란색 할당
    spotLights[0].position = cam.pos + vec3(0.7, 0.7, 0); // 조명 위치는 카메라 위치에서 좌상단으로 각각 0.7씩만큼 이동함
    spotLights[0].intensity = 1.0;
    spotLights[0].direction = vec3(0, 0, -1);
    spotLights[0].cutoff = glm::cos(glm::radians(35.0f));
    
    spotLights[1].color = vec3(0, 1, 1); // 조명색상 청록색 할당
    spotLights[1].position = cam.pos + vec3(0.0, -0.5, 0); // 조명 위치는 카메라 위치에서 0.5만큼 아래로 내림
    spotLights[1].intensity = 1.0;
    spotLights[1].direction = vec3(0, 0, -1);
    spotLights[1].cutoff = glm::cos(glm::radians(15.0f));
    
    // 투영행렬 계산
    float aspect = 1024.0f / 768.0f; // main.cpp 에서 정의한 윈도우 실행창 사이즈를 기준으로 원근투영행렬의 종횡비(aspect)값을 계산함.
    mat4 proj = perspective(cam.fov, aspect, 0.01f, 10.0f); // glm::perspective() 내장함수를 사용해 원근투영행렬 계산.
    
    // 카메라 변환시키는 뷰행렬 계산. 이동행렬만 적용
    mat4 view = inverse(translate(cam.pos)); // 뷰행렬은 카메라 움직임에 반대방향으로 나머지 대상들을 움직이는 변환행렬이므로, glm::inverse() 내장함수로 역행렬을 구해야 함.
    
    // 이후의 연산은 shield 메쉬 드로우 함수와 water 메쉬 드로우 함수로 쪼개서 추출함.
    drawShield(proj, view);
    drawWater(proj, view);
    drawSkybox(proj, view); // cubeMesh 메쉬 드로우 함수를 추출하여 정의한 뒤 호출함.
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

/**
 스카이박스 원근분할 무효화를 위한 깊이비교모드 변경
 
 스카이박스는 씬에서 볼 수 있는 대상 중 가장 멀리있는 대상이지만,
 투영공간에서 유일하게 원근분할 ( 원근 분할 설명은 p.260 ~ 261 참고) 이 적용이 안되어야 함.
 왜냐면, 스카이박스는 무한히 뻗어나가는 하늘을 보여줘야 하기 때문에, 여기에 원근효과를 적용하는 건 적절치 않음.
 
 따라서, 스카이박스는 버텍스 셰이더에서 w컴포넌트와 z컴포넌트를 일치시켜 원근분할을 무효화하는데,
 이 때, 원근분할로 변환된 NDC 좌표계 상에서는 기본적으로 좌표값이 1.0 보다 작은 버텍스만 그릴 수 있게 되어있음.
 (이러한 깊이 모드를 GL_LESS 라고 함.)
 
 그런데, 스카이박스는 Z / W = 1.0 이기 때문에
 GL_LESS 깊이비교모드 상에서는 화면에 그려지지 않을거임.
 
 따라서, OpenGL 함수인 glDepthFunc() 를 통해 깊이비교모드를
 GL_LEQUAL 로 일시적으로 변경함으로써,
 NDC 좌표계 상에서 z값이 1.0보다 같거나 작은 대상까지 그리게 하도록 변경함.
 
 이렇게 하면 스카이박스까지는 그려질 거니까 괜찮지만,
 일반적으로 사용하는 깊이비교모드는 아니기 때문에,
 혹시나 다른 부분에서 문제가 발생할 여지를 줄이기 위해
 
 스카이박스를 전부 그리고 나면 맨 마지막에
 깊이비교모드를 GL_LESS 로 원상복귀시킴.
 */

/**
 #define 매크로 상수
 
 c++ 에서는 프로그램 내에서 컴파일러가
 매크로를 만났을 때 대체할 문자열 또는 숫자값을 정의할 때 사용함.
 
 컴파일러가 컴파일 시작 전,
 #define 으로 정의된 기호를 지정된 숫자나 문자열로
 변환시킬 수 있도록 함.
 
 매크로는 변수가 아니므로, 런타임 이전에
 컴파일러가 해당 기호를 마주치면 지정된 값으로
 변환해주는 형태이므로 변수보다 처리속도가 빠르고,
 코드의 가독성 및 유지보수를 향상시키는 이점이 있다고 함.
 */
