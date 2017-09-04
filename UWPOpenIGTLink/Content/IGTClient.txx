/*====================================================================
Copyright(c) 2017 Adam Rankin


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files(the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and / or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
====================================================================*/

namespace UWPOpenIGTLink
{
  //----------------------------------------------------------------------------
  template<typename MessageTypePointer> double IGTClient::GetLatestTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    if (m_receiveMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto riter = m_receiveMessages.rbegin(); riter != m_receiveMessages.rend(); riter++)
      {
        if (dynamic_cast<MessageTypePointer*>(riter->GetPointer()) != nullptr)
        {
          (*riter)->GetTimeStamp(ts);
          return ts->GetTimeStamp();
        }
      }
    }
    return -1.0;
  }

  //----------------------------------------------------------------------------
  template<typename MessageTypePointer> double IGTClient::GetOldestTimestamp() const
  {
    // Retrieve the next available tracked frame reply
    if (m_receiveMessages.size() > 0)
    {
      igtl::TimeStamp::Pointer ts = igtl::TimeStamp::New();
      for (auto iter = m_receiveMessages.begin(); iter != m_receiveMessages.end(); iter++)
      {
        if (dynamic_cast<MessageTypePointer*>(iter->GetPointer()) != nullptr)
        {
          (*iter)->GetTimeStamp(ts);
          return ts->GetTimeStamp();
        }
      }
    }
    return -1.0;
  }
}