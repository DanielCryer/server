#ifndef HANDLERS_H
#define HANDLERS_H

#include "httplib.h"

namespace Handlers {

    void registrationHandler(const httplib::Request& req, httplib::Response& res);
    void authorizationHandler(const httplib::Request& req, httplib::Response& res);
    void gettingRequestsHandler(const httplib::Request& req, httplib::Response& res);
    void gettingUserRequestsHandler(const httplib::Request& req, httplib::Response& res);
    void gettingUserDealsByRequestHandler(const httplib::Request& req, httplib::Response& res);
    void creationRequestHandler(const httplib::Request& req, httplib::Response& res);
    void cancelRequestHandler(const httplib::Request& req, httplib::Response& res);
    void pingHandler(const httplib::Request& req, httplib::Response& res);
}


#endif // HANDLERS_H
