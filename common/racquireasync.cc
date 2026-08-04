#include "config.h" // IWYU pragma: associated

#include "racquireasync.h"

#include <queue>

// Fetcher message

struct MessageMediaChange
{
   std::string media;
   std::string drive;
};
struct MessageIMSHit
{
   pkgAcquire::ItemDesc item;
};
struct MessageFetch
{
   pkgAcquire::ItemDesc item;
};
struct MessageDone
{
   pkgAcquire::ItemDesc item;
};
struct MessageFail
{
   pkgAcquire::ItemDesc item;
};
struct MessageStart
{};
struct MessageStop
{};
struct MessageFinish
{
   std::any result;
};
using FetcherMessage = std::variant<MessageMediaChange,
                                    MessageIMSHit,
                                    MessageFetch,
                                    MessageDone,
                                    MessageFail,
                                    MessageStart,
                                    MessageStop,
                                    MessageFinish>;

template <typename T> class EventQueue
{
 public:
   void push(T event)
   {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(std::move(event));
   }

   std::optional<T> poll()
   {
      std::lock_guard<std::mutex> lock(mutex_);

      if (queue_.empty())
         return std::nullopt;

      auto event = std::move(queue_.front());
      queue_.pop();
      return event;
   }

   T pop()
   {
      while (true) {
         if (auto answer = poll()) {
            return answer.value();
         } else {
            usleep(100000);
         }
      }
   }

 private:
   std::mutex mutex_;
   std::queue<T> queue_;
};

class pkgAcquireStatusQueue : public pkgAcquireStatus
{
 public:
   virtual bool MediaChange(std::string Media, std::string Drive) override
   {
      messageQueue.push(MessageMediaChange{Media, Drive});
      return answerQueue.pop();
   }
   virtual void IMSHit(pkgAcquire::ItemDesc &Itm) override
   {
      messageQueue.push(MessageIMSHit{Itm});
   }
   virtual void Fetch(pkgAcquire::ItemDesc &Itm) override
   {
      messageQueue.push(MessageFetch{Itm});
   }
   virtual void Done(pkgAcquire::ItemDesc &Itm) override
   {
      messageQueue.push(MessageDone{Itm});
   }
   virtual void Fail(pkgAcquire::ItemDesc &Itm) override
   {
      messageQueue.push(MessageFail{Itm});
   }
   virtual void Start() override
   {
      messageQueue.push(MessageStart{});
   }
   virtual void Stop() override
   {
      messageQueue.push(MessageStop{});
   }

   explicit pkgAcquireStatusQueue(EventQueue<FetcherMessage> &messageQueue,
                                  EventQueue<bool> &answerQueue)
      : messageQueue(messageQueue), answerQueue(answerQueue)
   {}

 private:
   EventQueue<FetcherMessage> &messageQueue;
   EventQueue<bool> &answerQueue;
};

task<std::any> runWithStatusAsyncAny(
   std::function<std::any(pkgAcquireStatus &)> &&body,
   RPkgAcquireStatusAsync *status)
{
   EventQueue<FetcherMessage> messageQueue;
   EventQueue<bool> answerQueue;
   pkgAcquireStatusQueue statusQueue{messageQueue, answerQueue};

   std::thread worker([&]() {
      auto result = body(statusQueue);
      messageQueue.push(FetcherMessage{MessageFinish{result}});
   });

   while (true) {
      if (auto event = messageQueue.poll()) {
         std::optional<std::any> result = co_await std::visit(
            [status,
             &answerQueue](auto &&event) -> task<std::optional<std::any>> {
               using T = std::decay_t<decltype(event)>;
               if constexpr (std::is_same_v<T, MessageFinish>) {
                  co_return std::optional<std::any>{event.result};
               }
               if constexpr (std::is_same_v<T, MessageMediaChange>) {
                  bool result =
                     co_await status->MediaChange(event.media, event.drive);
                  answerQueue.push(result);
               } else if constexpr (std::is_same_v<T, MessageIMSHit>) {
                  co_await status->IMSHit(event.item);
               } else if constexpr (std::is_same_v<T, MessageFetch>) {
                  co_await status->Fetch(event.item);
               } else if constexpr (std::is_same_v<T, MessageDone>) {
                  co_await status->Done(event.item);
               } else if constexpr (std::is_same_v<T, MessageFail>) {
                  co_await status->Fail(event.item);
               } else if constexpr (std::is_same_v<T, MessageStart>) {
                  co_await status->Start();
               } else if constexpr (std::is_same_v<T, MessageStop>) {
                  co_await status->Stop();
               }
               co_return std::optional<std::any>{};
            },
            event.value());
         if (result) {
            worker.join();
            co_return result.value();
         }
      } else {
         //  co_await sleep_ms{100};
      }
   }
}
