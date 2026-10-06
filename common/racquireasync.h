#pragma once

#include "coroutines.h"
#include <apt-pkg/acquire.h>
#include <any>
#include <functional>

class RPkgAcquireStatusAsync
{
 public:
   [[nodiscard]] virtual task<bool> MediaChange(std::string Media,
                                                std::string Drive) = 0;
   [[nodiscard]] virtual task<void> IMSHit(pkgAcquire::ItemDesc &Itm) = 0;
   [[nodiscard]] virtual task<void> Fetch(pkgAcquire::ItemDesc &Itm) = 0;
   [[nodiscard]] virtual task<void> Done(pkgAcquire::ItemDesc &Itm) = 0;
   [[nodiscard]] virtual task<void> Fail(pkgAcquire::ItemDesc &Itm) = 0;
   [[nodiscard]] virtual task<void> Start() = 0;
   [[nodiscard]] virtual task<void> Stop() = 0;
};

[[nodiscard]] task<std::any> runWithStatusAsyncAny(
   std::function<std::any(pkgAcquireStatus &)> &&body,
   RPkgAcquireStatusAsync *status);

template <typename R>
[[nodiscard]] task<R> runWithStatusAsync(
   std::function<R(pkgAcquireStatus &)> &&body,
   RPkgAcquireStatusAsync *status)
{
   auto anyBody = [&](pkgAcquireStatus &status) -> std::any {
      return body(status);
   };
   auto result = co_await runWithStatusAsyncAny(std::move(anyBody), status);
   co_return std::any_cast<R>(result);
}

[[nodiscard]] inline task<pkgAcquire::RunResult> acquireRunAsync(
   pkgAcquire *acquire,
   RPkgAcquireStatusAsync *status,
   int PulseInterval = 500000)
{
   pkgAcquire::RunResult result =
      co_await runWithStatusAsync<pkgAcquire::RunResult>(
         [acquire,
          PulseInterval](pkgAcquireStatus &status) -> pkgAcquire::RunResult {
            acquire->SetLog(&status);
            pkgAcquire::RunResult result = acquire->Run(
#ifndef HAVE_RPM
               PulseInterval
#endif
            );
            acquire->SetLog(nullptr);
            return result;
         },
         status);
   co_return result;
}
