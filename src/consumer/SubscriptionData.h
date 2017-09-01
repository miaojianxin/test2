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
#if!defined __SUBSCRIPTIONDATA_H__
#define __SUBSCRIPTIONDATA_H__

#include <string>
#include <set>

class SubscriptionData
{
public:
	SubscriptionData();
	SubscriptionData(const std::string& topic, const std::string& subString);

	std::string getTopic()const;
	void setTopic(const std::string& topic);

	std::string getSubString()const;
	void setSubString(const std::string& subString);

	const std::set<std::string>& getTagsSet()const;
	void setTagsSet(const std::set<std::string>& tagsSet);

	long long getSubVersion()const;
	void setSubVersion(long long subVersion);

	const std::set<int>& getCodeSet() const;
	void setCodeSet(const std::set<int>& codeSet);

	int hashCode();

	bool operator==(const SubscriptionData& other);
	bool operator<(const SubscriptionData& other)const;

    SubscriptionData& operator=(const SubscriptionData& other);

public:
	static std::string SUB_ALL;

private:
	std::string m_topic;
	std::string m_subString;
	std::set<std::string> m_tagsSet;
	std::set<int> m_codeSet;
	long long m_subVersion ;

    friend class FilterAPI;
    friend class PullAPIWrapper;
};

#endif
