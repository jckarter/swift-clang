#ifndef LLVM_CLANG_CLANGSERVICE_SUPPORT_INPROCESSCLIENT_H
#define LLVM_CLANG_CLANGSERVICE_SUPPORT_INPROCESSCLIENT_H

#include "clang-service/Client.h"
#include "clang-service/Service.h"
#include "clang-service/Support/LiveData.h"

namespace ClangService {

class InProcessClient : public Client {
  Service CS;

public:
  Value request(Value Request) override {
    LiveData LD;
    return CS.handle(std::move(Request), LD).clone();
  }

  void request(Value Request, std::function<void(Value)> Handler) override {
    Handler(request(std::move(Request)));
  }
};

} // end namespace ClangService

#endif // LLVM_CLANG_CLANGSERVICE_SUPPORT_INPROCESSCLIENT_H
