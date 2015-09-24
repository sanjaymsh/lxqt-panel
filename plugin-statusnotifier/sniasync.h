/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * http://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Palo Kisa <palo.kisa@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#if !defined(SNIASYNC_H)
#define SNIASYNC_H

#include <functional>
#include "statusnotifieriteminterface.h"

template<typename> struct remove_class_type; // undefined
template<typename C, typename R, typename... ArgTypes>
struct  remove_class_type<R (C::*)(ArgTypes...)> { using type = R(ArgTypes...); };
template<typename C, typename R, typename... ArgTypes>
struct remove_class_type<R (C::*)(ArgTypes...) const> { using type = R(ArgTypes...); };

template <typename> struct lambda_signature; //undefined
template <typename L>
struct lambda_signature : public remove_class_type<decltype(&L::operator())> {};

template <typename> struct is_valid_signature : public std::false_type {};
template <typename Arg>
struct is_valid_signature<void (Arg)> : public std::true_type {};
template <typename L>
struct is_valid_lambda : public is_valid_signature<typename lambda_signature<L>::type> {};

class SniAsync : public QObject
{
    Q_OBJECT
public:
    SniAsync(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    template <typename F>
    inline void propertyGetAsync(QString const &name, F finished)
    {
        static_assert(is_valid_lambda<F>::value, "need lambda [] (Arg) -> void");
        connect(new QDBusPendingCallWatcher{asyncPropGet(name), this},
                &QDBusPendingCallWatcher::finished,
                [this, finished, name] (QDBusPendingCallWatcher * call)
                {
                    QDBusPendingReply<QVariant> reply = *call;
                    if (reply.isError())
                        qDebug() << "Error on DBus request:" << reply.error();
                    finished(qdbus_cast<typename std::function<typename lambda_signature<F>::type>::argument_type>(reply.value()));
                    call->deleteLater();
                }
        );
    }

    //exposed methods from org::kde::StatusNotifierItem
    inline QString service() const { return mSni.service(); }

public slots:
    //Forwarded slots from org::kde::StatusNotifierItem
    inline QDBusPendingReply<> Activate(int x, int y) { return mSni.Activate(x, y); }
    inline QDBusPendingReply<> ContextMenu(int x, int y) { return mSni.ContextMenu(x, y); }
    inline QDBusPendingReply<> Scroll(int delta, const QString &orientation) { return mSni.Scroll(delta, orientation); }
    inline QDBusPendingReply<> SecondaryActivate(int x, int y) { return mSni.SecondaryActivate(x, y); }

signals:
    //Forwarded signals from org::kde::StatusNotifierItem
    void NewAttentionIcon();
    void NewIcon();
    void NewOverlayIcon();
    void NewStatus(const QString &status);
    void NewTitle();
    void NewToolTip();

private:
    QDBusPendingReply<QDBusVariant> asyncPropGet(QString const & property);

private:
    org::kde::StatusNotifierItem mSni;

};

#endif
