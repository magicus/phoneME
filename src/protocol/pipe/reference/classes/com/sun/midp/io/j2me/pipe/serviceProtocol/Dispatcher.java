package com.sun.midp.io.j2me.pipe.serviceProtocol;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.services.SystemService;
import com.sun.midp.services.SystemServiceConnection;

class Dispatcher implements SystemService {

    private static final boolean DEBUG = true;
    private ServerEndpoint servers = null;
    private ClientEndpoint clients = null;

    Dispatcher(SecurityToken token) {
    }

    public String getServiceID() {
        return PipeServiceProtocol.SERVICE_ID;
    }

    public void start() {
    }

    public void stop() {
    }

    public void acceptConnection(SystemServiceConnection connection) {
        if (DEBUG)
            PipeServiceProtocol.debugPrintS(" Dispatcher.acceptConnection " + connection);
        UserListener listener = new UserListener(connection, this);
        connection.setConnectionListener(listener);
    }

    synchronized void addServerEndpoint(ServerEndpoint point) {
        point.next = servers;
        servers = point;
    }

    synchronized void removeServerEndpoint(ServerEndpoint point) {
        if (servers == point) {
            servers = (ServerEndpoint) point.next;
        } else if (servers != null) {
            removeEndpointFrom(point, servers);
        }

    }

    synchronized void addClientEndpoint(ClientEndpoint point) {
        point.next = clients;
        clients = point;
    }

    synchronized void removeClientEndpoint(ClientEndpoint point) {
        if (clients == point) {
            clients = (ClientEndpoint) point.next;
        } else if (clients != null) {
            removeEndpointFrom(point, clients);
        }

    }

    private synchronized void removeEndpointFrom(Endpoint point, Endpoint list) {
        for (Endpoint cur = list; cur.next != null; cur = cur.next) {
            if (cur.next == point) {
                cur.next = point.next;
            }
        }
    }

    synchronized void removeAllEndpoints(UserListener listener) {
        if (servers != null) {
        }
    }

    synchronized ServerEndpoint getServerEndpoint(String serverName, String serverVersion) {
        int version = PipeServiceProtocol.parseVersion(serverVersion);

        if (DEBUG)
            PipeServiceProtocol.debugPrintS(" searching for endpoint for pipe server " + serverName + 
                    ' ' + serverVersion + '(' + version + ')');
        for (ServerEndpoint point = servers; point != null; point = (ServerEndpoint) point.next) {
            if (point.suitableForClient(serverName, version))
                return point;
        }

        return null;
    }

    synchronized Endpoint getEndpoint(long endpointId) {
        if (DEBUG)
            PipeServiceProtocol.debugPrintS(" searching for endpoint id " + endpointId);

        Endpoint point;
        for (point = servers; point != null && point.getId() != endpointId; point = point.next) {
            //
        }
        if (point == null) {
            for (point = clients; point != null && point.getId() != endpointId; point = point.next) {
                //
            }
        }

        return point;
    }
}
