#include <assert.h>
#include <poll.h>

#include "Channel.h"
#include "EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN |POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    :tied_(false), 
    eventHandling_(false){
    addedToLoop_(false),
    fd_(fd),
    events_(0),
    revents_(0),
    index_(-1),
    loop_(loop){

}

Channel::~Channel(){
    assert(!eventHandling_);
}

void Channel::update(){
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void Channel::remove(){
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp t){
    std::shared_ptr<void> guard;
    if(tied_){
        // weak_ptr --> shared_ptr
        guard = tie_.lock();
        if(guard){
            handleEventWithGuard(t);
        }
    }
    else{
        handleEventWithGuard(t);
    }
}

void Channel::handleEventWithGuard(Timestamp t){
    eventHandling_ = true;

    if((revents_ & POLLHUP) && !(revents_ & POLLIN)){
        if(closeCallback_){
            closeCallback_();
        }
    }

    if(revents_ & POLLNVAL){
        // LOG_WARN
    }

    if(revents_ & (POLLERR | POLLNVAL)){
        if(errorCallback_){
            errorCallback_();
        }
    }

    if(revents_ & (POLLIN | POLLPRI | POLLRDHUP)){
        if(readCallback_){
            readCallback_(t);
        }
    }

    if(revents_ & (POLLOUT)){
        if(writeCallback_){
            writeCallback_();
        }
    }
    eventHandling_ = false;
}

void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}
