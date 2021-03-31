
#ifndef MyController_hpp
#define MyController_hpp

#define SERVER_CONF_FILE_PATH "/home/xavier/github/example-libressl/conf.d/server.json"
#define IMAGE_FILE_PATH "/home/xavier/github/example-libressl/image/"

#include "client/MySecureApiClient.hpp"
#include "dto/MyDTOs.hpp"

#include "oatpp/web/client/RequestExecutor.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

#include "halcon/Classifyer.hpp"
#include "halcon/MyDictionary.cpp"

#include <chrono>
#include <iostream>
#include <sys/time.h>
#include <ctime>

#include OATPP_CODEGEN_BEGIN(ApiController) //<--- Begin codegen

/**
 *  EXAMPLE ApiController
 *  Basic examples of howto create ENDPOINTs
 *  More details on oatpp.io
 */
class MyController : public oatpp::web::server::api::ApiController {
private:
    constexpr static const char* TAG = "MyController";
    MyDictionary myDictionary; //compile error
    oatpp::data::mapping::type::DTOWrapper<ServerConfigObj> serverConfigObj;
    bool saveImage = true;
    
    static int miliSecTime() {
        using std::cout; using std::endl;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;
        using std::chrono::seconds;
        using std::chrono::system_clock;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
    
protected:
  typedef MyController __ControllerType;
public:
  OATPP_COMPONENT(std::shared_ptr<MySecureApiClient>, mySecureApiClient);
protected:
  MyController(const std::shared_ptr<ObjectMapper>& objectMapper)
    : oatpp::web::server::api::ApiController(objectMapper)
    {
        auto jsonObjectMapper = oatpp::parser::json::mapping::ObjectMapper::createShared();
        if (oatpp::String jsonStr = oatpp::base::StrBuffer::loadFromFile(SERVER_CONF_FILE_PATH)) {
                OATPP_LOGD(TAG, "Server configuration: %s", jsonStr->c_str());
                serverConfigObj = jsonObjectMapper->readFromString<oatpp::Object<ServerConfigObj>>(jsonStr);
        } else OATPP_LOGD(TAG, "%s NOT existing", SERVER_CONF_FILE_PATH);
    }
public:
  
  /**
   *  Inject @objectMapper component here as default parameter
   *  Do not return bare Controllable* object! use shared_ptr!
   */
  static std::shared_ptr<MyController> createShared(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>,
                                                                    objectMapper)){
    return std::shared_ptr<MyController>(new MyController(objectMapper));
  }
  
  /**
   *  Hello World endpoint Coroutine mapped to the "/" (root)
   */
  ENDPOINT_ASYNC("GET", "/", Root) {
    
    ENDPOINT_ASYNC_INIT(Root)
    
    /**
     *  Coroutine entrypoint act()
     *  returns Action (what to do next)
     */
    Action act() override {
      auto dto = HelloDto::createShared();
      dto->message = "Hello Async!";
      dto->server = Header::Value::SERVER;
      dto->userAgent = request->getHeader(Header::USER_AGENT);
      return _return(controller->createDtoResponse(Status::CODE_200, dto));
    }
    
  };
  
  /**
   *  Echo body endpoint Coroutine. Mapped to "/body/string".
   *  Returns body received in the request
   */
  ENDPOINT_ASYNC("GET", "/body/string", EchoStringBody) {
    
    ENDPOINT_ASYNC_INIT(EchoStringBody)
    
    Action act() override {
      /* return Action to start child coroutine to read body */
      return request->readBodyToStringAsync().callbackTo(&EchoStringBody::returnResponse);
    }
    
    Action returnResponse(const oatpp::String& body){
      /* return Action to return created OutgoingResponse */
      return _return(controller->createResponse(Status::CODE_200, body));
    }
    
  };
  
  /**
   *  Echo body endpoint Coroutine. Mapped to "/body/dto".
   *  Deserialize DTO reveived, and return same DTO
   *  Returns body as MessageDto received in the request
   */
  ENDPOINT_ASYNC("GET", "/body/dto", EchoDtoBody) {
    
    ENDPOINT_ASYNC_INIT(EchoDtoBody)
    
    Action act() override {
      return request->readBodyToDtoAsync<oatpp::Object<MessageDto>>(controller->getDefaultObjectMapper()).callbackTo(&EchoDtoBody::returnResponse);
    }
    
    Action returnResponse(const oatpp::Object<MessageDto>& body){
      return _return(controller->createDtoResponse(Status::CODE_200, body));
    }
    
  };
  
  ENDPOINT_ASYNC("GET", "/api/get", TestApiGet) {
    
    ENDPOINT_ASYNC_INIT(TestApiGet)
    
    Action act() override {
      return controller->mySecureApiClient->apiGetAsync().callbackTo(&TestApiGet::onResponse);
    }
    
    Action onResponse(const std::shared_ptr<IncomingResponse>& response){
      return response->readBodyToStringAsync().callbackTo(&TestApiGet::returnResult);
    }
    
    Action returnResult(const oatpp::String& body) {
      return _return(controller->createResponse(Status::CODE_200, body));
    }
    
  };
    
  ENDPOINT_ASYNC("PUT", "/detectObject", DetectObject) {
    /*  curl -X PUT "https://localhost:8443/detectObject" --insecure -T <file name> */
          
    ENDPOINT_ASYNC_INIT(DetectObject)
    
    Action act() override {
        OATPP_LOGD(TAG, "DetectObject::act()");
        return request->readBodyToStringAsync().callbackTo(&DetectObject::returnResponse);
    }
    
    Action returnResponse(const oatpp::String& jpegData){
        /* return Action to return created OutgoingResponse */
        OATPP_LOGD(TAG, "DetectObject::returnResponse()");
        if (controller->serverConfigObj->saveImage) {
            std::string fileName = IMAGE_FILE_PATH;
            fileName = fileName + "image" + std::to_string(miliSecTime()) + ".jpeg";
            jpegData->saveToFile(fileName.c_str());
            OATPP_LOGD(TAG, "an image data has been stored at %s", fileName.c_str());
        }
        try {
          auto myDictionary = controller->myDictionary;
          Classifyer *myClassifyer = new Classifyer(&myDictionary);
          myClassifyer -> execute(jpegData->c_str());
          
          auto responseDto = ImgBoundingBoxObj::createShared();
          responseDto->boxList = {};
            if (myClassifyer->boxVector.size() == 0) {
                  auto box = BoundingBoxObj::createShared();
                  box->sex = -1;
                  box->score = 0;
                  box->top = 0;
                  box->left = 0;
                  box->height = 0;
                  box->width = 0;
                  //boxIndex = 0;
                  //OATPP_LOGD(TAG, "URI=/chick/%d, {sex:%d,score:%d,position:{top:%d,left:%d,height:%d,width:%d}}", *deviceId, *box->sex, *box->score, *box->top, *box->left, *box->height, *box->width);
                  responseDto->boxList->push_back(box);

            } else {
               for (int boxIndex = 0; boxIndex < myClassifyer->boxVector.size(); ++boxIndex) {
                  auto box = BoundingBoxObj::createShared();
                  box->sex =  myClassifyer->boxVector[boxIndex][0];
                  box->score = myClassifyer->boxVector[boxIndex][1] * 100;
                  box->top = myClassifyer->boxVector[boxIndex][2] * 100;
                  box->left = myClassifyer->boxVector[boxIndex][3] * 100;
                  box->height = myClassifyer->boxVector[boxIndex][4] * 100;
                  box->width = myClassifyer->boxVector[boxIndex][5] * 100;
                  //OATPP_LOGD(TAG, "URI=/chick/%d, boxVector={sex:%5.3f,score:%5.3f,position:{top:%5.3f,left:%5.3f,height:%5.3f,width:%5.3f}}", *deviceId, myClassifyer->boxVector[boxIndex][0], myClassifyer->boxVector[boxIndex][1], myClassifyer->boxVector[boxIndex][2], myClassifyer->boxVector[boxIndex][3], myClassifyer->boxVector[boxIndex][4], myClassifyer->boxVector[boxIndex][5]);
                  //OATPP_LOGD(TAG, "URI=/chick/%d, BoundingBoxObj={sex:%d,score:%d,position:{top:%d,left:%d,height:%d,width:%d}}", *deviceId, *box->sex, *box->score, *box->top, *box->left, *box->height, *box->width);
                  responseDto->boxList->push_back(box);
                } 
            }
            delete myClassifyer;
            
            auto response = controller->createDtoResponse(Status::CODE_200, responseDto);
            response->putHeader(Header::CONTENT_TYPE, "application/json");
            return _return(response);
          }
          catch (std::runtime_error e) {
              const char* text = e.what();
          }
        }
    };
    
    ENDPOINT_ASYNC("GET", "/testDetectObject", TestDetectObject) {
    
    ENDPOINT_ASYNC_INIT(TestDetectObject)
    
    Action act() override {
      oatpp::String binData = "aaaabbbbbbbbbbbbbbbbbbbbbbbbbbb";
      return controller->mySecureApiClient->apiDetectObjectAsync(binData).callbackTo(&TestDetectObject::onResponse);
    }
    
    Action onResponse(const std::shared_ptr<IncomingResponse>& response){
      return response->readBodyToStringAsync().callbackTo(&TestDetectObject::returnResult);
    }
    
    Action returnResult(const oatpp::String& body) {
      return _return(controller->createResponse(Status::CODE_200, body));
    }
    
  };
  
};

#include OATPP_CODEGEN_END(ApiController) //<--- End codegen

#endif /* MyController_hpp */
