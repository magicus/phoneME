package com.sun.midp.push.persistence;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.sun.midp.push.controller.ConnectionInfo;

/** In memory impl of <code>Store</code>. */
public final class InMemoryStore implements Store {
    /**
     *  Alarms' records.
     *
     * <p>
     * Implemented as suiteID -> (midlet -> time)
     * </p>
     */
    private final Map alarms = new HashMap();

    /** {@inheritDoc}. */
    public void addAlarm(final int midletSuiteID, final String midlet,
            final long time) throws IOException {
        final Integer id = new Integer(midletSuiteID);
        final Long t = new Long(time);

        final Map suiteInfo = (Map) alarms.get(id);
        if (suiteInfo == null) {
            final Map si = new HashMap();
            si.put(midlet, t);
            alarms.put(id, si);
        } else {
            suiteInfo.put(midlet, t);
        }
    }

    /** {@inheritDoc}. */
    public void listAlarms(final AlarmsConsumer alarmsConsumer) {
        for (Iterator it = alarms.entrySet().iterator(); it.hasNext();) {
            final Map.Entry entry = (Map.Entry) it.next();
            final Integer id = (Integer) entry.getKey();
            final Map suiteInfo = (Map) entry.getValue();

            if (!suiteInfo.isEmpty()) {
                alarmsConsumer.consume(id.intValue(), suiteInfo);
            }
        }
    }

    /** {@inheritDoc}. */
    public void removeAlarm(final int midletSuiteID, final String midlet)
            throws IOException {
        final Integer id = new Integer(midletSuiteID);

        final Map suiteInfo = (Map) alarms.get(id);
        if (suiteInfo == null) {
            throw new RuntimeException("Internal invariant broken:"
                    + " trying to remove alarm for midlet with no alarms");
        }

        suiteInfo.remove(midlet);
    }

    /**
     *  Connections' records.
     *
     * <p>
     * Implemented as suiteID -> collection of ConnectionInfo
     * </p>
     */
    private final Map connections = new HashMap();

    /**
     * Converts an array into a collection.
     *
     * @param cns connections to convert
     * @return collection
     */
    private Collection asCollection(final ConnectionInfo [] cns) {
        return new HashSet(Arrays.asList(cns));
    }

    /** {@inheritDoc}. */
    public void addConnection(final int midletSuiteID,
            final ConnectionInfo connection) throws IOException {
        final Integer id = new Integer(midletSuiteID);

        final Collection cns = (Collection) connections.get(id);
        if (cns == null) {
            connections.put(id,
                    asCollection(new ConnectionInfo [] {connection}));
        } else {
            cns.add(connection);
        }
    }

    /** {@inheritDoc}. */
    public void addConnections(final int midletSuiteID,
            final ConnectionInfo[] connections) throws IOException {
        final Integer id = new Integer(midletSuiteID);
        final Collection newCns = asCollection(connections);

        final List cns = (List) this.connections.get(id);
        if (cns == null) {
            this.connections.put(id, newCns);
        } else {
            cns.addAll(newCns);
        }
    }

    /** {@inheritDoc}. */
    public void listConnections(
            final ConnectionsConsumer connectionsConsumer) {
        for (Iterator it = connections.entrySet().iterator();
                it.hasNext();) {
            final Map.Entry entry = (Map.Entry) it.next();
            final Integer id = (Integer) entry.getKey();
            final Collection cns = (Collection) entry.getValue();

            if (!cns.isEmpty()) {
                final ConnectionInfo [] c = (ConnectionInfo[])
                    cns.toArray(new ConnectionInfo[cns.size()]);
                connectionsConsumer.consume(id.intValue(), c);
            }
        }
    }

    /** {@inheritDoc}. */
    public void removeConnection(final int midletSuiteID,
            final ConnectionInfo connection) throws IOException {
        final Integer id = new Integer(midletSuiteID);

        final Collection cns = (Collection) connections.get(id);
        if (cns == null) {
            throw new RuntimeException("Internal invariant broken:"
                    + " trying to remove a connection for midlet with"
                    + " no registered connections");
        }

        cns.remove(connection);
    }

    /** {@inheritDoc}. */
    public void removeConnections(final int midletSuiteID)
            throws IOException {
        final Integer id = new Integer(midletSuiteID);
        connections.remove(id);
    }
}