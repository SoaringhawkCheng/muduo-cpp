// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/poller/PollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Channel.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop)
  : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  // XXX pollfds_ shouldn't change
  // numEvents表示发生事件的数目
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);//pollfds_是关注事件的列表
  //返回值是当前有事件发生的文件描述符的数量
  int savedErrno = errno;
  Timestamp now(Timestamp::now());//获取当前时间
  if (numEvents > 0)//有事件发生
  {
    LOG_TRACE << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);//将发生事件对应的处理事件的通道加入激活通道列表
  }
  else if (numEvents == 0)//无事件发生
  {
    LOG_TRACE << " nothing happended";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "PollPoller::poll()";
    }
  }
  return now;
}

void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const
{
  for (PollFdList::const_iterator pfd = pollfds_.begin();
      pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)// numEvents为0，即所有事件都处理完
    {
      --numEvents;// 每处理一件事情，numEvents减少1
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);//根据发生的事情的描述符找到对应的通道
      assert(ch != channels_.end());
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);//设置发生的事件
      // pfd->revents = 0;
      activeChannels->push_back(channel);//将通道加入激活通道列表
    }
  }
}

//updatechannel和removechannel负责添加，更新和删除channel
//pollfds保存要监听的事件列表
//channels保存文件描述符和对应的通道的映射关系
void PollPoller::updateChannel(Channel* channel)//传入要更新的通道
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)//通道所处理的事件不在事件列表中
  {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;//设置并添加
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);//
    int idx = static_cast<int>(pollfds_.size())-1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;//添加描述符与通道的映射关系
  }
  else//通道所处理的事件在事件列表中
  {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd& pfd = pollfds_[idx];//从事件列表中取出事件
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    //更新事件信息
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent())
    {
      // ignore this pollfd
      pfd.fd = -channel->fd()-1;
    }
  }
}

void PollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1); (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1)
  {
    pollfds_.pop_back();
  }
  else
  {
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1;
    }
    channels_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
}

