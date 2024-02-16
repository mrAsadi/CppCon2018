#include "shared_state.hpp"
#include "websocket_session.hpp"

shared_state::
    shared_state(std::string doc_root)
    : doc_root_(std::move(doc_root))
{
}

void shared_state::
    connect(const std::string &connection_id, websocket_session* session)
{
    sessions_[connection_id] = session;
    send(connection_id, "you connected as :" + connection_id);
}

void shared_state::
    disconnect(const std::string &connection_id)
{
    if (connection_id != "")
    {
        auto it = sessions_.find(connection_id);
        if (it != sessions_.end())
        {
            sessions_.erase(it);
            broadcast("a client disconnected :" + connection_id);
        }
    }
}

    void shared_state::
        send(const std::string &connection_id, const std::string &message)
    {
        auto const session = get(connection_id);
        if (session != nullptr)
        {
            auto const ss = boost::make_shared<std::string const>(std::move(message));
            session->send(ss);
        }
    }

    void shared_state::
        broadcast(const std::string &message)
    {
        for (const auto &entry : sessions_)
        {
            websocket_session* session = entry.second;
            if (session != nullptr)
            {
                auto const ss = boost::make_shared<std::string const>(std::move(message));
                session->send(ss);
            }
        }
    }

    websocket_session* shared_state::
        get(const std::string &connection_id)
    {
        auto it = sessions_.find(connection_id);
        return (it != sessions_.end()) ? it->second : nullptr;
    }

    void shared_state::joinGroup(const std::string &connection_id, const std::string &group_name)
    {
        if (hasJoinPermission(connection_id, group_name))
        {
            groups_[group_name].push_back(connection_id);
            connectionToGroup_[connection_id] = group_name;
            send(connection_id, "You joined group: " + group_name);
        }
        else
        {
            send(connection_id, "You have not permission to join group: " + group_name);
        }
    }

    void shared_state::leaveGroup(const std::string &connection_id, const std::string &group_name)
    {
        auto it = connectionToGroup_.find(connection_id);
        if (it != connectionToGroup_.end() && it->second == group_name)
        {
            groups_[group_name].erase(std::remove(groups_[group_name].begin(), groups_[group_name].end(), connection_id), groups_[group_name].end());
            connectionToGroup_.erase(it);

            // Send a message to the user confirming group leave
            send(connection_id, "You left group: " + group_name);
        }
    }

    void shared_state::sendToGroup(const std::string &group_name, const std::string &sender_connection_id, const std::string &message)
    {
        if (hasSendPermission(sender_connection_id, group_name))
        {
            auto members = getGroupMembers(group_name);
            for (const auto &member : members)
            {
                if (member != sender_connection_id)
                {
                    send(member, message);
                }
            }
        }
        else
        {
            send(sender_connection_id, "You have not permission to send group: " + group_name);
        }
    }

    void shared_state::sendToConnection(const std::string &sender_connection_id, const std::string &receiver_connection_id, const std::string &message)
    {
        auto receiver_session = get(receiver_connection_id);
        if (receiver_session != nullptr)
        {
            auto ss = boost::make_shared<std::string const>(message);
            receiver_session->send(ss);
        }
    }

    std::vector<std::string> shared_state::getGroupMembers(const std::string &group_name)
    {
        auto it = groups_.find(group_name);
        return (it != groups_.end()) ? it->second : std::vector<std::string>();
    }

    std::string shared_state::grantJoinPermissions(const std::string &connection_id, const std::vector<std::string> &permissions)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Check if the connection exists
        auto it = sessions_.find(connection_id);
        if (it == sessions_.end())
        {
            return "Join Connection not found for :" + connection_id + ".\n";
        }
        joinPermissions_[connection_id] = permissions;
        return "Join permission granted";
    }

    void shared_state::revokeJoinPermissions(const std::string &connection_id, const std::vector<std::string> &permissions)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &permission : permissions)
        {
            auto &connectionPermissions = joinPermissions_[connection_id];
            auto it = std::find(connectionPermissions.begin(), connectionPermissions.end(), permission);
            if (it != connectionPermissions.end())
            {
                connectionPermissions.erase(it);
            }
        }
    }

    bool shared_state::hasJoinPermission(const std::string &connection_id, const std::string &permission)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Check if the connection has the specified permission
        auto it = joinPermissions_.find(connection_id);
        if (it != joinPermissions_.end())
        {
            const std::vector<std::string> &permissions = it->second;
            return std::find(permissions.begin(), permissions.end(), permission) != permissions.end();
        }
        return false;
    }

    std::string shared_state::grantSendPermissions(const std::string &connection_id, const std::vector<std::string> &permissions)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = sessions_.find(connection_id);
        if (it == sessions_.end())
        {
            return "Send Connection not found for :" + connection_id + ".\n";
        }
        sendPermissions_[connection_id] = permissions;
        return "Send permission granted";
    }

    void shared_state::revokeSendPermissions(const std::string &connection_id, const std::vector<std::string> &permissions)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto &permission : permissions)
        {
            auto &connectionPermissions = sendPermissions_[connection_id];
            auto it = std::find(connectionPermissions.begin(), connectionPermissions.end(), permission);
            if (it != connectionPermissions.end())
            {
                connectionPermissions.erase(it);
            }
        }
    }

    bool shared_state::hasSendPermission(const std::string &connection_id, const std::string &permission)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // Check if the connection has the specified permission
        auto it = sendPermissions_.find(connection_id);
        if (it != sendPermissions_.end())
        {
            const std::vector<std::string> &permissions = it->second;
            return std::find(permissions.begin(), permissions.end(), permission) != permissions.end();
        }
        return false;
    }