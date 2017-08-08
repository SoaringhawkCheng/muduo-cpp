// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{
namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
 public:
  //boost::function是一个函数包装器，也即一个函数模板
  //用来代替拥有相同返回类型，相同参数类型，以及相同参数个数的各个不同函数
  //功能上类似函数指针，貌似默认模式化为NULL
  typedef boost::function<void()> EventCallback;
  typedef boost::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void handleEvent(Timestamp receiveTime);
  //设置回调函数
  void setReadCallback(const ReadEventCallback& cb)
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setCloseCallback(const EventCallback& cb)
  { closeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  void setReadCallback(ReadEventCallback&& cb)
  { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback&& cb)
  { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback&& cb)
  { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback&& cb)
  { errorCallback_ = std::move(cb); }
#endif

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in handleEvent.
  void tie(const boost::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; } // used by pollers
  // int revents() const { return revents_; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  //修改关注事件
  void enableReading() { events_ |= kReadEvent; update(); }
  void disableReading() { events_ &= ~kReadEvent; update(); }
  void enableWriting() { events_ |= kWriteEvent; update(); }
  void disableWriting() { events_ &= ~kWriteEvent; update(); }
  void disableAll() { events_ = kNoneEvent; update(); }
  //判断关注事件类型
  bool isWriting() const { return events_ & kWriteEvent; }
  bool isReading() const { return events_ & kReadEvent; }

  // for Poller
  //处理通道所对应的关注事件，在关注时间列表中的下标
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  string reventsToString() const;
  string eventsToString() const;

  void doNotLogHup() { logHup_ = false; }

  EventLoop* ownerLoop() { return loop_; }//返回所属的事件循环
  void remove();//从事件循环中移除通道

 private:
  static string eventsToString(int fd, int ev);

  void update();//在事件循环中更新该通道
  void handleEventWithGuard(Timestamp receiveTime);

  static const int kNoneEvent;//无事件 宏状态
  static const int kReadEvent;//读事件 宏状态
  static const int kWriteEvent;//写事件 宏状态

  EventLoop* loop_;//所属的事件循环
  const int  fd_;//监听的描述符
  int        events_;// 关心的io事件
  int        revents_; // it's the received event types of epoll or poll
  int        index_; //
  bool       logHup_;

  boost::weak_ptr<void> tie_;
  bool tied_;
  bool eventHandling_;
  bool addedToLoop_;//该通道是否加入到事件循环中
  //一系列回调函数
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

}
}
#endif  // MUDUO_NET_CHANNEL_H
