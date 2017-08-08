// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>

#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

namespace muduo
{
namespace net
{

class Channel;

///
/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
/// 虚基类，见PollPoller
class Poller : boost::noncopyable
{
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  /// 轮询获取当前活动的io事件
  /// 填充调用方传入的激活通道列表activeChannels
  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void updateChannel(Channel* channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void removeChannel(Channel* channel) = 0;

  virtual bool hasChannel(Channel* channel) const;//通道映射表里有该通道

  static Poller* newDefaultPoller(EventLoop* loop);

  void assertInLoopThread() const
  {
    ownerLoop_->assertInLoopThread();
  }

 protected:
  typedef std::map<int, Channel*> ChannelMap;//通道映射表
  ChannelMap channels_;//用于文件描述符与通道的映射

 private:
  EventLoop* ownerLoop_;//轮询器是事件循环的间接成员，传入所属事件循环的指针
};

}
}
#endif  // MUDUO_NET_POLLER_H
