/**
* Copyright (C) 2013 kangliqiang ,kangliq@163.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#if!defined __HEARTBEATDATA_H__
#define __HEARTBEATDATA_H__

#include <string>
#include <set>
#include <sstream>

#include "ConsumeType.h"
#include "SubscriptionData.h"
#include "RemotingSerializable.h"

typedef struct ConsumerData
{
	std::string groupName;
	ConsumeType consumeType;
	MessageModel messageModel;
	ConsumeFromWhere consumeFromWhere;
	std::set<SubscriptionData> subscriptionDataSet;
	bool operator < (const ConsumerData& cd)const
	{
		return groupName<cd.groupName;
	}
} ConsumerData;

typedef struct  ProducerData
{
	std::string groupName;
	bool operator < (const ProducerData& pd)const
	{
		return groupName<pd.groupName;
	}

} ProducerData;


class HeartbeatData : public RemotingSerializable
{
public:
	void Encode(std::string& outData)
	{
		std::stringstream ss;

    	ss<<"{"<<"\"clientID\":"<<"\""<<m_clientID<<"\"";
        if(!m_producerDataSet.empty())
        {
            ss<<",\"producerDataSet\":"<<"[";
            std::set<ProducerData>::iterator it = m_producerDataSet.begin();
    		for (;it != m_producerDataSet.end();it++)
    		{
    		    if(it == m_producerDataSet.begin())
                {
                    ss<<"{\"groupName\":\""<<it->groupName<<"\"}";
                }
                else
                {
                    ss<<",{\"groupName\":\""<<it->groupName<<"\"}";
                }
    		}
            ss<<"]";
        }

        if(!m_consumerDataSet.empty())
        {
            std::string consumeWhere;
            std::string consumeType;
            std::string messModel;
            ss<<",\"consumerDataSet\":"<<"[";
            std::set<ConsumerData>::iterator it = m_consumerDataSet.begin();
    		for (;it != m_consumerDataSet.end();it++)
    		{
                if(CONSUME_FROM_FIRST_OFFSET == it->consumeFromWhere)
                {
                    consumeWhere = "CONSUME_FROM_FIRST_OFFSET";
                }
                else if(CONSUME_FROM_LAST_OFFSET == it->consumeFromWhere)
                {
                    consumeWhere = "CONSUME_FROM_LAST_OFFSET";
                }
                else if(CONSUME_FROM_TIMESTAMP == it->consumeFromWhere)
                {
                    consumeWhere = "CONSUME_FROM_TIMESTAMP";
                }
                else
                {
                    consumeWhere = "UNKOWN";
                }

                if(CONSUME_PASSIVELY == it->consumeType)
                {
                    consumeType = "CONSUME_PASSIVELY";
                }
                else if(CONSUME_ACTIVELY == it->consumeType)
                {
                    consumeType = "CONSUME_ACTIVELY";
                }
                else
                {
                    consumeType = "UNKOWN";
                }

                if(CLUSTERING == it->messageModel)
                {
                    messModel = "CLUSTERING";
                }
                else if(BROADCASTING == it->messageModel)
                {
                    messModel = "BROADCASTING";
                }
                else
                {
                    messModel = "UNKOWN";
                }

                if(it != m_consumerDataSet.begin())
                {
                    ss<<",";
                }
                ss<<"{\"consumeFromWhere\":\""<<consumeWhere<<"\","
                    <<"\"consumeType\":\""<<consumeType<<"\","
                    <<"\"groupName\":\""<<it->groupName<<"\","                    
                    <<"\"messageModel\":\""<<messModel<<"\","
                    <<"\"subscriptionDataSet\":[";
                std::set<SubscriptionData>::iterator itSub = it->subscriptionDataSet.begin();
    		    for (;itSub != it->subscriptionDataSet.end();itSub++)
                {
                    if(itSub != it->subscriptionDataSet.begin())
                    {
                        ss<<",";
                    }
                    ss<<"{\"classFilterMode\":false,"
                        <<"\"codeSet\":[";
                    std::set<int>::const_iterator itCode = itSub->getCodeSet().begin();
                    for (;itCode != itSub->getCodeSet().end();itCode++)
                    {
                        if(itCode != itSub->getCodeSet().begin())
                        {
                            ss<<",";
                        }
                        ss<<*itCode;
                    }
                    ss<<"],\"subString\":\""<<itSub->getSubString()<<"\","
                        <<"\"subVersion\":"<<itSub->getSubVersion()<<","
                        <<"\"tagsSet\":[";
                    
                    std::set<std::string>::const_iterator itTag = itSub->getTagsSet().begin();
                    for (;itTag != itSub->getTagsSet().end();itTag++)
                    {
                        if(itTag != itSub->getTagsSet().begin())
                        {
                            ss<<",";
                        }
                        ss<<"\""<<*itTag<<"\"";
                    }
                    ss<<"],\"topic\":\""<<itSub->getTopic()<<"\"}";                    
                }
                ss<<"],\"unitMode\":false}";                
    		}
            ss<<"]";
        }
        ss<<"}";

    	outData = ss.str();
	}

	std::string getClientID()
	{
		return m_clientID;
	}

	void setClientID(const std::string& clientID)
	{
		m_clientID = clientID;
	}

	std::set<ProducerData>& getProducerDataSet()
	{
		return m_producerDataSet;
	}

	void setProducerDataSet(const std::set<ProducerData>& producerDataSet)
	{
		m_producerDataSet = producerDataSet;
	}

	std::set<ConsumerData>& getConsumerDataSet()
	{
		return m_consumerDataSet;
	}

	void setConsumerDataSet(const std::set<ConsumerData>& consumerDataSet)
	{
		m_consumerDataSet = consumerDataSet;
	}

private:
	std::string m_clientID;
	std::set<ProducerData> m_producerDataSet;
	std::set<ConsumerData> m_consumerDataSet;
};

#endif
