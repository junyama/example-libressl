
#ifndef MySecureApiClient_hpp
#define MySecureApiClient_hpp

#include "oatpp/web/client/ApiClient.hpp"
#include "oatpp/core/data/mapping/type/Object.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(ApiClient)

class MySecureApiClient : public oatpp::web::client::ApiClient {
  
  API_CLIENT_INIT(MySecureApiClient)
  
  API_CALL("GET", "/get", apiGet)
  
  API_CALL_ASYNC("GET", "/get", apiGetAsync)
      
  API_CALL_ASYNC("PUT", "/detectObject", apiDetectObjectAsync, BODY_STRING(String, body))

};

#include OATPP_CODEGEN_END(ApiClient)

#endif /* MySecureApiClient_hpp */
