#ifndef IR_WEBSOCKET_SERVER_SHARED_STATE_HPP
#define IR_WEBSOCKET_SERVER_SHARED_STATE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/smart_ptr.hpp>
#include <mutex>
#include <vector>

// Forward declaration
class websocket_session;

// Represents the shared server state
class shared_state
{
    std::string doc_root_;

    // This simple method of tracking
    // sessions only works with an implicit
    // strand (i.e. a single-threaded server)
    std::mutex mutex_;
    std::unordered_map<std::string, websocket_session*> sessions_;
    std::unordered_map<std::string, std::vector<std::string>> groups_;
    std::unordered_map<std::string, std::string> connectionToGroup_;
    std::unordered_map<std::string, std::vector<std::string>> sendPermissions_;
    std::unordered_map<std::string, std::vector<std::string>> joinPermissions_;
    std::vector<std::string> getGroupMembers(const std::string& group_name);

public:
    explicit shared_state(std::string doc_root);

    std::string const &
    doc_root() const noexcept
    {
        return doc_root_;
    }

    void connect(const std::string &connection_id,websocket_session* session);
    void disconnect(const std::string &connection_id);
    void send(const std::string &connection_id,const std::string &message);
    void broadcast(const std::string &message);
    websocket_session* get(const std::string &connection_id);
    void joinGroup(const std::string& connection_id, const std::string& group_name);
    void leaveGroup(const std::string& connection_id, const std::string& group_name);
    void sendToGroup(const std::string& group_name, const std::string& sender_connection_id, const std::string& message);
    void sendToConnection(const std::string& sender_connection_id, const std::string& receiver_connection_id, const std::string& message);
    std::string grantJoinPermissions(const std::string &connection_id, const std::vector<std::string> &permissions);
    void revokeJoinPermissions(const std::string &connection_id, const std::vector<std::string> &permissions);
    bool hasJoinPermission(const std::string &connection_id, const std::string &permission);
    std::string grantSendPermissions(const std::string &connection_id, const std::vector<std::string> &permissions);
    void revokeSendPermissions(const std::string &connection_id, const std::vector<std::string> &permissions);
    bool hasSendPermission(const std::string &connection_id, const std::string &permission);


};

#endif
