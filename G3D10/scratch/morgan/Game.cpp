#if 0

typedef ID int32;
enum MessageType {CREATE, DESTROY, MOVE};

class Server {

    class Player : public ReferenceCountedObject {
    public:
        shared_ptr<NetConnection>       connection;
        CFrame                          position;
        ID                              id;
        ...
    };

    Array<shared_ptr<Player> >          m_playerArray;

    shared_ptr<NetServer>               m_server;

    ID                                  m_nextUniqueID;

public:

    void onNetwork() {
        // See if there are any new clients
        for (NetNewClientIterator2 client = m_server->incomingClientIterator(); client.isValid(); ++client) {
            // Create a unique ID, add to the player array
            ...
        }

        for (int p = 0; p < m_playerArray.size(); ++p) {
            shared_ptr<Player> player = m_playerArray[p];
            shared_ptr<NetConnection> connection = player->connection;
           
            // The following simply returns no messages if not connected
            for (NetMessageIterator msg = connection->incomingMessageIterator(); msg.isValid(); ++msg) {
                // The only messages supported right now are moves, so no need to read a type
                player->position.deserialize(msg.binaryInput());

                // Tell the other players
                BinaryOutput bo(G3D_LITTLE_ENDIAN);
                bo.writeInt32(MOVE);
                bo.writeInt32(player->id);
                player->position.serialize(bo);
                m_server->broadcast->send(bo);
            }

            // Remove dropped clients
            if (connection->status() == NetConnection::DISCONNECTED) {
                m_playerArray.fastRemove(p);
                --p;
            }
        }
    }

    void onSimulation(...) {
        ...
    }
};


class Client {

    class Player : public ReferenceCountedObject {
    public:
        CFrame                          position;
        ID                              id;
        ...
    };

    shared_ptr<Player>                  m_localPlayer;
    Table<ID, shared_ptr<Player> >      m_playerTable;
    shared_ptr<NetConnection>           m_connection;

public:

    void onNetwork() {
        // The following simply returns no messages if not connected
        for (NetMessageIterator msg = m_connection->incomingMessageIterator(); msg.isValid(); ++msg) {
            BinaryInput& bi = msg.binaryInput();

            const MessageType messageType = bi.readInt32();
            const ID id = bi.readInt32();

            // Ignore messages about me
            if (id != m_localPlayer->id) {

                switch (messageType) {
                case CREATE:
                    m_playerTable.set(id, Player::create(id, Point3(bi)));
                    break;

                case DESTROY:
                    m_playerTable.remove(id);
                    break;

                case MOVE:
                    m_playerTable[id]->position.deserialize(bi);
                    break;
                } // switch
            } // if not local
        } // for each message
    }


    void onSimulation(...) {
        m_localPlayer->position += ...;

        // Tell the server that I've moved
        BinaryOutput bo(G3D_LITTLE_ENDIAN);
        m_connection->send(bo);
    }

    ...
};

#endif
