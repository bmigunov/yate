/**
 * qt4client.h
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * A Qt-4 based universal telephony client
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2006 Null Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __QT4CLIENT_H
#define __QT4CLIENT_H

#include <yatecbase.h>

#ifdef _WINDOWS

#ifdef LIBYQT4_EXPORTS
#define YQT4_API __declspec(dllexport)
#else
#ifndef LIBYQT4_STATIC
#define YQT4_API __declspec(dllimport)
#endif
#endif

#endif /* _WINDOWS */
                                          
#ifndef YQT4_API
#define YQT4_API
#endif

#undef open
#undef read
#undef close
#undef write
#undef mkdir
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define QT_NO_DEBUG
#define QT_DLL
#define QT_GUI_LIB
#define QT_CORE_LIB
#define QT_THREAD_SUPPORT

#include <QtGui>
#include <QSound>

namespace TelEngine {

class QtEventProxy;                      // Proxy to global QT events
class QtClient;                          // The QT based client
class QtDriver;                          // The QT based telephony driver
class QtWindow;                          // A QT window
class QtDialog;                          // A custom modal dialog
class QtUIWidgetItemProps;               // Widget container item properties
class QtUIWidget;                        // A widget container
class QtCustomObject;                    // A custom QT object
class QtCustomWidget;                    // A custom QT widget
class QtTable;                           // A custom QT table widget
class QtSound;                           // A QT client sound

// Macro used to get a QT object's name
// Can't use an inline function: the QByteArray object returned by toUtf8()
//  would be destroyed on exit
#define YQT_OBJECT_NAME(qobject) ((qobject) ? (qobject)->objectName().toUtf8().constData() : "")

/**
 * Proxy to global QT events
 * @short A QT proxy class
 */
class YQT4_API QtEventProxy : public QObject, public GenObject
{
    YCLASS(QtEventProxy,GenObject)
    Q_CLASSINFO("QtEventProxy","Yate")
    Q_OBJECT

public:
    enum Type {
	Timer,
	AllHidden,
    };

    /**
     * Constructor
     * @param Event type
     * @param pointer to QT application when needed
     */
    QtEventProxy(Type type, QApplication* app = 0);

    /**
     * Get a string representation of this object
     * @return Object's name
     */
    virtual const String& toString() const
	{ return m_name; }

private slots:
    void timerTick();                    // Idle timer
    void allHidden();                    // All windows closed notification

private:
    String m_name;                       // Object name
};

class YQT4_API QtClient : public Client
{
    friend class QtWindow;
public:
    /**
     * Generic position flags
     */
    enum QtClientPos {
	PosNone   = 0,
	PosLeft   = 0x01,
	PosRight  = 0x02,
	PosTop    = 0x04,
	PosBottom = 0x08,
	// Corners
	CornerTopLeft = PosTop | PosLeft,
	CornerTopRight = PosTop | PosRight,
	CornerBottomLeft = PosBottom | PosLeft,
	CornerBottomRight = PosBottom | PosRight,
    };

    QtClient();
    virtual ~QtClient();
    virtual void run();
    virtual void cleanup();
    virtual void main();
    virtual void lock();
    virtual void unlock();
    virtual void allHidden();
    virtual bool createWindow(const String& name,
	const String& alias = String::empty());
    virtual bool action(Window* wnd, const String& name, NamedList* params = 0);
    virtual void quit() {
	    if (m_app)
		m_app->quit();
	    else
		Engine::halt(0);
	}

    /**
     * Open an URL (link)
     * @param url The URL to open
     * @return True on success
     */
    virtual bool openUrl(const String& url)
	{ return QDesktopServices::openUrl(QUrl(setUtf8(url))); }

    /**
     * Show a file save/open dialog window. If the list of parameters contains an 'action'
     *  parameter, an action will be raised when the dialog will be closed. The action's
     *  parameter list pointer will be non 0 if the dialog was accepted and 0 if cancelled.
     *  The list will contain one or more 'file' parameter(s) with selected file(s)
     * @param parent Dialog window's parent
     * @param params Dialog window's params. Parameters that can be specified include 'caption',
     *  'dir', 'filters', 'selectedfilter', 'choosefile'
     * @return True on success (the dialog was opened)
     */
    virtual bool chooseFile(Window* parent, NamedList& params);

    /**
     * Create a sound object. Append it to the global list
     * @param name The name of sound object
     * @param file The file to play (should contain the whole path and the file name)
     * @param device Optional device used to play the file. Set to 0 to use the default one
     * @return True on success, false if a sound with the given name already exists
     */
    virtual bool createSound(const char* name, const char* file, const char* device = 0);

    /**
     * Build a date/time string from UTC time
     * @param dest Destination string
     * @param secs Seconds since EPOCH
     * @param format Format string used to build the destination
     * @param utc True to build UTC time instead of local time
     * @return True on success
     */
    virtual bool formatDateTime(String& dest, unsigned int secs, const char* format,
	bool utc = false);

    /**
     * Build a date/time QT string from UTC time
     * @param secs Seconds since EPOCH
     * @param format Format string
     * @param utc True to build UTC time instead of local time
     * @return The formated string
     */
    static QString formatDateTime(unsigned int secs, const char* format,
	bool utc = false);

    /**
     * Get an UTF8 representation of a QT string
     * @param dest Destination string
     * @param src Source QT string
     */
    static inline void getUtf8(String& dest, const QString& src)
	{ dest = src.toUtf8().constData(); }

    /**
     * Get an UTF8 representation of a QT string and add it to a list of parameters
     * @param dest Destination list
     * @param param Parameter name/value
     * @param src Source QT string
     * @param setValue True to set the QT string as parameter value, false to set it
     *  as parameter name
     */
    static inline void getUtf8(NamedList& dest, const char* param,
	const QString& src, bool setValue = true) {
	    if (setValue)
		dest.addParam(param,src.toUtf8().constData());
	    else
		dest.addParam(src.toUtf8().constData(),param);
	}

    /**
     * Set a QT string from an UTF8 char buffer
     * @param str The buffer
     * @return A QT string filled with the buffer
     */
    static inline QString setUtf8(const char* str)
	{ return QString::fromUtf8(TelEngine::c_safe(str)); }

    /**
     * Retrieve an object's QtWindow parent
     * @param obj The object
     * @return QtWindow pointer or 0
     */
    static QtWindow* parentWindow(QObject* obj);

    /**
     * Set an object's property into parent window's section. Clear it on failure
     * @param obj The object
     * @param prop Property to save
     * @param owner Optional window owning the object
     * @return True on success
     */
    static bool saveProperty(QObject* obj, const String& prop, QtWindow* owner = 0);

    /**
     * Set or an object's property
     * @param obj The object
     * @param name Property's name
     * @param value Property's value
     * @return False if the property doesn't exist or has a type not supported by String
     */
    static bool setProperty(QObject* obj, const char* name, const String& value);

    /**
     * Get an object's property
     * @param obj The object
     * @param name Property's name
     * @param value Property's value
     * @return False if the property doesn't exist or has a type not supported by String
     */
    static bool getProperty(QObject* obj, const char* name, String& value);

    /**
     * Get an object's property and return its boolean conversion
     * @param obj The object
     * @param name Property name
     * @param defVal Default value to return if the property is not found or has
     *  invalid boolean value
     * @return The boolean conversion of the property or given default value
     */
    static inline bool getBoolProperty(QObject* obj, const char* name,
	bool defVal = false) {
	    String tmp;
	    if (!getProperty(obj,name,tmp))
		return defVal;
	    return tmp.toBoolean(defVal);
	}

    /**
     * Get an object's property and return its integer conversion
     * @param obj The object
     * @param name Property name
     * @param defVal Default value to return if the property is not found or has
     *  invalid integer value
     * @return The integer conversion of the property or given default value
     */
    static inline int getIntProperty(QObject* obj, const char* name,
	int defVal = 0) {
	    String tmp;
	    if (!getProperty(obj,name,tmp))
		return defVal;
	    return tmp.toInteger(defVal);
	}

    /**
     * Associate actions to buttons with '_yate_setaction' property set
     * @param parent Parent widget
     */
    static void setAction(QWidget* parent);

    /**
     * Check if an object has '_yate_noautoconnect' boolean property set to true
     * @param obj The object
     * @return True if the object don't have the property or its value is not a boolean 'true'
     */
    static inline bool autoConnect(QObject* obj)
	{ return !getBoolProperty(obj,"_yate_noautoconnect"); }

    /**
     * Retrieve an object's identity from '_yate_identity' property or object name
     * @param obj The object
     * @param ident String to be filled with object identity
     */
    static inline void getIdentity(QObject* obj, String& ident) {
	    if (obj && !(getProperty(obj,"_yate_identity",ident) && ident))
		getUtf8(ident,obj->objectName());
	}

    /**
     * Copy a string list to a list of parameters
     * @param dest Destination list
     * @param src Source string list
     */
    static void copyParams(NamedList& dest, const QStringList& src);

    /**
     * Copy a list of parameters to string list
     * @param dest Destination list
     * @param src Source list
     */
    static void copyParams(QStringList& dest, const NamedList& src);

    /**
     * Build QObject properties from list
     * @param obj The object
     * @param props Comma separated list of properties. Format: name=type
     */
    static void buildProps(QObject* obj, const String& props);

    /**
     * Build custom UI widgets from frames owned by a widget
     * @param parent Parent widget
     */
    static void buildFrameUiWidgets(QWidget* parent);

    /**
     * Build a menu object from a list of parameters.
     * Each menu item is indicated by a parameter starting with 'item:".
     * item:menu_name=Menu Text will create a menu item named 'menu_name' with 
     *  'Menu Text' as display name.
     * If the item parameter is a NamedPointer a submenu will be created.
     * Menu actions properties can be set from parameters with format:
     *  property:object_name:property_name=value
     * @param params The menu parameters. The list name is the object name
     * @param text The menu display text
     * @param receiver Object receiving menu actions
     * @param actionSlot The receiver's slot for menu signal triggered()
     * @param toggleSlot The receiver's slot for menu signal toggled()
     * @param aboutToShowSlot The receiver's slot for menu signal aboutToShow()
     * @param parent Optional widget parent
     * @return QMenu pointer or 0 if failed to build it
     */
    static QMenu* buildMenu(const NamedList& params, const char* text, QObject* receiver,
	 const char* actionSlot, const char* toggleSlot, QWidget* parent = 0,
	 const char* aboutToShowSlot = 0);

    /**
     * Insert a widget into another one replacing any existing children
     * @param parent Parent widget
     * @param child Widget to insert into parent
     * @return True on success
     */
    static bool setWidget(QWidget* parent, QWidget* child);

    /**
     * Set an object's image property from image file
     * @param obj The object
     * @param img Image file to load
     * @param fit True to adjust the image to target size if applicable (like
     *  a QLabel without scaled contents)
     * @return True on success
     */
    static bool setImage(QObject* obj, const String& img, bool fit = true);

    /**
     * Set an object's image property from raw data
     * @param obj The object
     * @param data The image data
     * @param format Image format if known
     * @param fit True to adjust the image to target size if applicable (like
     *  a QLabel without scaled contents)
     * @return True on success
     */
    static bool setImage(QObject* obj, const DataBlock& data,
	const String& format = String::empty(), bool fit = true);

    /**
     * Set an object's image property from QPixmap
     * @param obj The object
     * @param img The image
     * @param fit True to adjust the image to target size if applicable (like
     *  a QLabel without scaled contents)
     * @return True on success
     */
    static bool setImage(QObject* obj, const QPixmap& img, bool fit = true);

    /**
     * Filter key press events. Retrieve an action associated with the key.
     * Check if the object is allowed to process the key
     * @param obj The object
     * @param event QKeyEvent event to process
     * @param action Found action name
     * @param filter Filter key or let the object process it
     * @param parent Optional parent to look for the action and check its state
     * @return True if key and modifiers were matched against object properties
     *  (the action parameter may be empty if true is returned and the action is disabled)
     */
    static bool filterKeyEvent(QObject* obj, QKeyEvent* event, String& action,
	bool& filter, QObject* parent = 0);

    /**
     * Wrapper for QObject::connect() used to put a debug mesage on failure
     */
    static bool connectObjects(QObject* sender, const char* signal,
	 QObject* receiver, const char* slot);

    /**
     * Safely delete a QObject. Disconnect it, reset its parent, calls its deleteLater() method
     * @param obj The object to delete
     */
    static void deleteLater(QObject* obj);

    /**
     * Retrieve unavailable space position (if any) in the screen containing a given widget.
     * The positions are set using the difference between screen geometry and available geometry
     * @param w The widget
     * @param pos Unavailable screen space if any (QtClientPos combination)
     * @return Valid pointer to global desktop widget on success
     */
    static QDesktopWidget* getScreenUnavailPos(QWidget* w, int& pos);

    /**
     * Move a window to a specified position
     * @param w The window to move
     * @param pos A corner position
     */
    static void moveWindow(QtWindow* w, int pos);

protected:
    virtual void loadWindows(const char* file = 0);
private:
    QApplication* m_app;
    ObjList m_events;                    // Proxy events objects
};

class YQT4_API QtDriver : public ClientDriver
{
public:
    QtDriver();
    virtual ~QtDriver();
    virtual void initialize();
private:
    bool m_init;                         // Already initialized flag
};

class YQT4_API QtWindow : public QWidget, public Window
{
    YCLASS(QtWindow, Window)
    Q_CLASSINFO("QtWindow", "Yate")
    Q_OBJECT

    friend class QtClient;
public:
    QtWindow();
    QtWindow(const char* name, const char* description, const char* alias, QtWindow* parent = 0);
    virtual ~QtWindow();

    virtual void title(const String& text);
    virtual void context(const String& text);
    virtual bool setParams(const NamedList& params);
    virtual void setOver(const Window* parent);
    virtual bool hasElement(const String& name);
    virtual bool setActive(const String& name, bool active);
    virtual bool setFocus(const String& name, bool select = false);
    virtual bool setShow(const String& name, bool visible);

    /**
     * Set the displayed text of an element in the window
     * @param name Name of the element
     * @param text Text value to set in the element
     * @param richText True if the text contains format data
     * @return True if the operation was successfull
     */
    virtual bool setText(const String& name, const String& text,
	bool richText = false);

    virtual bool setCheck(const String& name, bool checked);
    virtual bool setSelect(const String& name, const String& item);
    virtual bool setUrgent(const String& name, bool urgent);

    virtual bool hasOption(const String& name, const String& item);
    virtual bool addOption(const String& name, const String& item, bool atStart = false, const String& text = String::empty());
    virtual bool delOption(const String& name, const String& item);
    virtual bool getOptions(const String& name, NamedList* items);

    /**
     * Append or insert text lines to a widget
     * @param name The name of the widget
     * @param lines List containing the lines
     * @param max The maximum number of lines allowed to be displayed. Set to 0 to ignore
     * @param atStart True to insert, false to append
     * @return True on success
     */
    virtual bool addLines(const String& name, const NamedList* lines, unsigned int max, 
	bool atStart = false);

    virtual bool addTableRow(const String& name, const String& item, const NamedList* data = 0, bool atStart = false);

    virtual bool setMultipleRows(const String& name, const NamedList& data, const String& prefix);

    /**
     * Insert a row into a table owned by this window
     * @param name Name of the element
     * @param item Name of the item to insert
     * @param before Name of the item to insert before
     * @param data Table's columns to set
     * @return True if the operation was successfull
     */
    virtual bool insertTableRow(const String& name, const String& item,
	const String& before, const NamedList* data = 0);

    virtual bool delTableRow(const String& name, const String& item);
    virtual bool setTableRow(const String& name, const String& item, const NamedList* data);
    virtual bool getTableRow(const String& name, const String& item, NamedList* data = 0);
    virtual bool clearTable(const String& name);

    /**
     * Set a table row or add a new one if not found
     * @param name Name of the element
     * @param item Table item to set/add
     * @param data Optional list of parameters used to set row data
     * @param atStart True to add item at start, false to add them to the end
     * @return True if the operation was successfull
     */
    virtual bool updateTableRow(const String& name, const String& item,
	const NamedList* data = 0, bool atStart = false);

    /**
     * Add or set one or more table row(s). Screen update is locked while changing the table.
     * Each data list element is a NamedPointer carrying a NamedList with item parameters.
     * The name of an element is the item to update.
     * Set element's value to boolean value 'true' to add a new item if not found
     *  or 'false' to set an existing one. Set it to empty string to delete the item
     * @param name Name of the table
     * @param data The list of items to add/set/delete
     * @param atStart True to add new items at start, false to add them to the end
     * @return True if the operation was successfull
     */
    virtual bool updateTableRows(const String& name, const NamedList* data,
	bool atStart = false);

    /**
     * Get an element's text
     * @param name Name of the element
     * @param text The destination string
     * @param richText True to get the element's roch text if supported.
     * @return True if the operation was successfull
     */
    virtual bool getText(const String& name, String& text, bool richText = false);

    virtual bool getCheck(const String& name, bool& checked);
    virtual bool getSelect(const String& name, String& item);

    /**
     * Build a menu from a list of parameters.
     * See Client::buildMenu() for more info
     * @param params Menu build parameters
     * @return True on success
     */
    virtual bool buildMenu(const NamedList& params);

    /**
     * Remove a menu from UI and memory
     * See Client::removeMenu() for more info
     * @param params Menu remove parameters
     * @return True on success
     */
    virtual bool removeMenu(const NamedList& params);

    /**
     * Set an element's image
     * @param name Name of the element
     * @param image Image to set
     * @return True on success
     */
    virtual bool setImage(const String& name, const String& image);

    /**
     * Set a property for this window or for a widget owned by it
     * @param name Name of the element
     * @param item Property's name
     * @param value Property's value
     * @return False if the property doesn't exist or has a type not supported by String
     */
    virtual bool setProperty(const String& name, const String& item, const String& value);

    /**
     * Get a property from this window or from a widget owned by it
     * @param name Name of the element
     * @param item Property's name
     * @param value Property's value
     * @return False if the property doesn't exist or has a type not supported by String
     */
    virtual bool getProperty(const String& name, const String& item, String& value);

    virtual void show();
    virtual void hide();
    virtual void size(int width, int height);
    virtual void move(int x, int y);
    virtual void moveRel(int dx, int dy);
    virtual bool related(const Window* wnd) const;
    virtual void menu(int x, int y) ;

    /**
     * Create a modal dialog
     * @param name Dialog name (resource config section)
     * @param title Dialog title
     * @param alias Optional dialog alias (used as dialog object name)
     * @param params Optional dialog parameters
     * @return True on success
     */
    virtual bool createDialog(const String& name, const String& title,
	const String& alias = String::empty(), const NamedList* params = 0);

    /**
     * Destroy a modal dialog
     * @param name Dialog name
     * @return True on success
     */
    virtual bool closeDialog(const String& name);

    /**
     * Connect an abstract button to window slots
     * @param b The button to connect
     * @return True on success
     */
    inline bool connectButton(QAbstractButton* b) {
	    if (!b)
		return false;
	    if (!b->isCheckable())
		return QtClient::connectObjects(b,SIGNAL(clicked()),this,SLOT(action()));
	    return QtClient::connectObjects(b,SIGNAL(toggled(bool)),this,SLOT(toggled(bool)));
	}

    /**
     * Connect an object's text changed signal to window's slot
     * @param obj The object to connect
     * @return True on success
     */
    bool connectTextChanged(QObject* obj);

    /**
     * Notify text changed to the client
     * @param obj The object sending the notification
     * @param text Optional object text
     */
    void notifyTextChanged(QObject* obj, const QString& text = QString());

    /**
     * Load a widget from file
     * @param fileName UI filename to load
     * @param parent The widget holding the loaded widget's contents
     * @param uiName The loaded widget's name (used for debug)
     * @param path Optional fileName path. Set to 0 to use the default one
     * @return QWidget pointer or 0 on failure 
     */
    static QWidget* loadUI(const char* fileName, QWidget* parent,
	const char* uiName, const char* path = 0);

    /**
     * Clear the UI cache
     * @param fileName Optional UI filename to clear. Clear all if 0
     */
    static void clearUICache(const char* fileName = 0);
    
    /**
     * Retrieve the parent window
     * @return QtWindow pointer or 0
     */
    inline QtWindow* parentWindow() const
	{ return qobject_cast<QtWindow*>(parentWidget() ? parentWidget()->window() : 0); }

    /**
     * Check if this window is shown normal (not maximixed, minimized or full screen)
     * @return True if the window is not maximixed, minimized or full screen
     */
    inline bool isShownNormal() const
	{ return !(isMaximized() || isMinimized() || isFullScreen()); }

protected:
    // Notify client on selection changes
    inline bool select(const String& name, const String& item,
	const String& text = String::empty()) {
	    if (!QtClient::self() || QtClient::changing())
		return false;
	    return QtClient::self()->select(this,name,item,text);
	}

    // Filter events to apply dynamic properties changes
    bool eventFilter(QObject* watched, QEvent* event);
    // Handle key pressed events
    void keyPressEvent(QKeyEvent* event);

public slots:
    void setVisible(bool visible);
    // A widget was double clicked
    void doubleClick();
    // A widget's selection changed
    void selectionChanged();
    // Clicked actions
    void action();
    // Toggled actions
    void toggled(bool);
    // System tray actions
    void sysTrayIconAction(QSystemTrayIcon::ActivationReason reason);
    // Choose file window was accepted
    void chooseFileAccepted();
    // Choose file window was cancelled
    void chooseFileRejected();
    // Text changed slot. Notify the client
    void textChanged(const QString& text)
	{ notifyTextChanged(sender(),text); }
    void textChanged()
	{ notifyTextChanged(sender()); }

private slots:
    void openUrl(const QString& link);

protected:
    virtual void doPopulate();
    virtual void doInit();
    // Methods inherited from QWidget
    virtual void moveEvent(QMoveEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual bool event(QEvent* ev);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    virtual void changeEvent(QEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* ev) {
	    if (handleContextMenuEvent(ev,wndWidget()))
		ev->accept();
	}
    // Get the widget with this window's content
    inline QWidget* wndWidget()
	{ return findChild<QWidget*>(m_widget); }
    // Handle context menu events. Return true if handled
    bool handleContextMenuEvent(QContextMenuEvent* event, QObject* obj);

    String m_description;
    String m_oldId;                      // Old id used to retreive the config section in .rc
    int m_x;
    int m_y;
    int m_width;                         // Client area width
    int m_height;                        // Client area height
    bool m_maximized;
    bool m_mainWindow;                   // Main window flag: close app when this window is closed
    QString m_widget;                    // The widget with window's content
    bool m_moving;                       // Flag used to move the window on mouse move event
    QPoint m_movePos;                    // Old position used when moving the window
};

/**
 * This class encapsulates a custom modal dialog window.
 * A dialog context can be set in '_yate_context' property
 * Actions triggered by dialogs have the following format: dialog:dialog_name:action_name.
 * The dialog will delete itself if an action is handled
 * @short A custom modal dialog
 */
class YQT4_API QtDialog : public QDialog
{
    Q_CLASSINFO("QtDialog","Yate")
    Q_OBJECT
    Q_PROPERTY(QString _yate_context READ context WRITE setContext(QString))
public:
    /**
     * Constructor
     * @param parent Parent widget
     */
    inline QtDialog(QWidget* parent)
	: QDialog(parent)
	{}

    /**
     * Destructor. Notify the client if not exiting
     */
    virtual ~QtDialog();

    /**
     * Retrieve the parent window
     * @return QtWindow pointer or 0
     */
    inline QtWindow* parentWindow() const
	{ return qobject_cast<QtWindow*>(parentWidget() ? parentWidget()->window() : 0); }

    /**
     * Initialize dialog. Load the widget.
     * Connect non checkable actions to own slot.
     * Connect checkable actions/buttons to parent window's slot
     * Display the dialog on success
     * @param name Object and config section name
     * @param title Window title
     * @param alias Object name to set if not empty
     * @param params Optional parent window parameters
     * @return True on success
     */
    bool show(const String& name, const String& title, const String& alias,
	const NamedList* params);

    /**
     * Retrieve the context property
     * @return The dialog context
     */
    QString context()
	{ return m_context; }

    /**
     * Set the dialog context
     * @param c The new dialog context
     */
    void setContext(QString c)
	{ m_context = c; }

    /**
     * Build an action's name
     * @param buf Destination buffer
     * @param action Action name
     * @return The destination string
     */
    inline String& buildActionName(String& buf, const String& action) {
	    buf = String("dialog:") + YQT_OBJECT_NAME(this) + ":" + action;
	    return buf;
	}

protected slots:
    // Notify client
    void action();

protected:
    // Destroy the dialog
    virtual void closeEvent(QCloseEvent* event);
    // Destroy the dialog
    virtual void reject();

    String m_notifyOnClose;              // Action to notify when closed
    QString m_context;                   // Dialog context
};

/**
 * This class holds data about a widget container item
 * @short Widget container item properties
 */
class QtUIWidgetItemProps : public String
{
public:
    /**
     * Constructor
     * @param type Item type
     */
    explicit inline QtUIWidgetItemProps(const String& type)
	: String(type)
	{}

    String m_ui;                         // Item UI file
    String m_styleSheet;                 // Item style sheet when not selected
    String m_selStyleSheet;              // Item selected style
};

/**
 * This class holds a basic widget container with functions to rename children
 * @short A widget container
 */
class YQT4_API QtUIWidget : public UIWidget
{
    YCLASS(QtUIWidget,UIWidget)
public:
    /**
     * Constructor
     * @param name Object name
     * @param params Object parameters
     * @param parent Optional parent
     */
    inline QtUIWidget(const char* name)
	: UIWidget(name)
	{}

    /**
     * Build a child name from this one
     * @param buf Destination buffer
     * @param item Child name
     * @return The destination buffer
     */
    inline String& buildChildName(String& buf, const String& item)
	{ return buildChildName(buf,name(),item); }

    /**
     * Build a container QString child name
     * @param item Child name
     * @return QString child name
     */
    inline QString buildQChildName(const String& item)
	{ return buildQChildName(name(),item); }

    /**
     * Retrieve item type definition
     * @param type Item type name
     * @return QtUIWidgetItemProps pointer or 0
     */
    inline QtUIWidgetItemProps* getItemProps(const String& type) {
	    ObjList* o = m_itemProps.find(type);
	    return o ? static_cast<QtUIWidgetItemProps*>(o->get()) : 0;
	}

    /**
     * Retrieve item type definition from [type:]value. Create it if not found
     * @param in Input string
     * @param value Item property value
     * @return QtUIWidgetItemProps pointer or 0
     */
    virtual QtUIWidgetItemProps* getItemProps(QString& in, String& value);

    /**
     * Retrieve the list of properties to save
     * @return The list of properties to save
     */
    QStringList saveProps()
	{ return m_saveProps; }

    /**
     * Set the list of properties to save
     * @param list The new list of properties to save
     */
    void setSaveProps(QStringList list)
	{ m_saveProps = list; }

    /**
     * Retrieve a QObject descendent of this object
     * @return QObject pointer or 0
     */
    virtual QObject* getQObject()
	{ return 0; }

    /**
     * Retrieve the window owning this object
     * @return QtWindow pointer or 0
     */
    virtual QtWindow* getWindow()
	{ return QtClient::parentWindow(getQObject()); }

    /**
     * Set widget's parameters.
     * Handle an 'applyall' parameter carrying a NamedList to apply to all items
     * @param params List of parameters
     * @return True if all parameters could be set
     */
    virtual bool setParams(const NamedList& params);

    /**
     * Retrieve a QObject list containing container items
     * @return The list of container items
     */
    virtual QList<QObject*> getContainerItems()
	{ return QList<QObject*>(); }

    /**
     * Find an item widget by id
     * @param id Item id
     * @return QWidget pointer or 0
     */
    virtual QWidget* findItem(const String& id);

    /**
     * Apply a list of parameters to all container items
     * @return The list of parameters to apply
     */
    virtual void applyAllParams(const NamedList& params);

    /**
     * Retrieve the object identity from '_yate_identity' property or name
     * Retrieve the object item from '_yate_widgetlistitem' property.
     * Set 'identity' to object_identity[:item_name]
     * @param obj The object
     * @param identiy Destination buffer
     */
    virtual void getIdentity(QObject* obj, String& identity);

    /**
     * Update an item object and children from a list a parameters
     * @param parent Parent object
     * @param params The list of parameters
     * @return True on success
     */
    virtual bool setParams(QObject* parent, const NamedList& params);

    /**
     * Get an item object's parameters
     * @param parent The object
     * @param params Parameter list
     * @return True on success
     */
    virtual bool getParams(QObject* parent, NamedList& params);

    /**
     * Retrieve object slots
     * @param actionSlot Action (triggerred) slot
     * @param toggleSlot Toggled slot
     * @param selectSlot Selection change slot
     */
    virtual void getSlots(String& actionSlot, String& toggleSlot, String& selectSlot) {
	    actionSlot = SLOT(itemChildAction());
	    toggleSlot = SLOT(itemChildToggle(bool));
	    selectSlot = SLOT(itemChildSelect());
	}

    /**
     * Select an item by its index
     * @param index Item index to select
     * @return True on success
     */
    virtual bool setSelectIndex(int index)
	{ return false; }

    /**
     * Retrieve the 0 based index of the current item
     * @return The index of the current item (-1 on error or container empty)
     */
    virtual int currentItemIndex()
	{ return -1; }

    /**
     * Retrieve the number of items in container
     * @return The number of items in container (-1 on error)
     */
    virtual int itemCount()
	{ return -1; }

    /**
     * Build a child's widget menu. Connect actions to container slots
     * @param w The widget
     * @param params Menu params
     * @param child Optional widget child target
     * @param set True to set the menu, false to build it and just return it
     * @return QMenu pointer or 0
     */
    QMenu* buildWidgetItemMenu(QWidget* w, const NamedList* params,
	const String& child = String::empty(), bool set = true);

    /**
     * Build a container child name
     * @param buf Destination buffer
     * @param name Container widget name
     * @param item Child name
     * @return The destination buffer
     */
    static inline String& buildChildName(String& buf, const String& name,
	const String& item) {
	    buf = name + "_" + item;
	    return buf;
	}

    /**
     * Build a container child name
     * @param name Container widget name
     * @param item Child name
     * @return QString child name
     */
    static inline QString buildQChildName(const QString& name, const QString& item)
	{ return name + "_" + item; }

    /**
     * Build a container QString child name
     * @param name Container widget name
     * @param item Child name
     * @return QString child name
     */
    static inline QString buildQChildName(const String& name, const String& item) {
	    String buf;
	    return QtClient::setUtf8(buildChildName(buf,name,item));
	}

    /**
     * Set the list item id property to a list item object
     * @param obj The object
     * @param item Item id property value
     */
    static inline void setListItemIdProp(QObject* obj, const QString& item)
	{ obj->setProperty("_yate_widgetlistitemid",QVariant(item)); }

    /**
     * Retrieve the list item id property from a list item object
     * @param obj The object
     * @param item Destination string
     */
    static inline void getListItemIdProp(QObject* obj, String& item)
	{ QtClient::getProperty(obj,"_yate_widgetlistitemid",item); }

    /**
     * Set the list item property for an item's child object
     * @param obj The object
     * @param item Item property value
     */
    static inline void setListItemProp(QObject* obj, const QString& item)
	{ obj->setProperty("_yate_widgetlistitem",QVariant(item)); }
	
    /**
     * Retrieve the list item property from an item's child object
     * @param obj The object
     * @param item Destination string
     */
    static inline void getListItemProp(QObject* obj, String& item)
	{ QtClient::getProperty(obj,"_yate_widgetlistitem",item); }

    /**
     * Retrieve the top level QtUIWidget container parent of an object
     * @param obj The object
     * @return QtUIWidget pointer or 0 if not found
     */
    static QtUIWidget* container(QObject* obj);

protected:
    /**
     * Default constructor
     */
    QtUIWidget()
	{}

    /**
     * Initialize navigation controls
     * @param params Parameter list
     */
    void initNavigation(const NamedList& params);

    /**
     * Update navigation controls
     */
    void updateNavigation();

    /**
     * Trigger a custom action from an item. Build a list of parameters containing
     *  the 'item' and the 'list' object identity
     * @param item The item id
     * @param action The action name to trigger
     * @param sender Optional sender (set it to 0 to use getQObject())
     */
    void triggerAction(const String& item, const String& action, QObject* sender = 0);

    /**
     * Handle a child's action. Retrieve the object identity (using getIdentity()) and
     *  notify the action 'sender_identity:sender_item_name' to the client
     * Internally handle next/prev actions if set
     * @param sender The sender
     */
    virtual void onAction(QObject* sender);

    /**
     * Handle a child's action. Retrieve the object identity (using getIdentity()) and
     *  notify the toggled 'sender_identity:sender_item_name' event to the client
     * @param sender The sender
     * @param on Toggle status
     */
    virtual void onToggle(QObject* sender, bool on);

    /**
     * Handle a child's selection change. Retrieve the object identity and
     *  notify the select 'sender_identity:sender_item_name' event to the client.
     * @param sender The sender
     * @param item Optional selected item if any. Set it to 0 to detect it
     */
    virtual void onSelect(QObject* sender, const String* item = 0);

    /**
     * Load an item's widget. Rename children.
     * Set '_yate_widgetlistitemid' widget property to given name.
     * Set '_yate_widgetlistitem' to item for each child.
     * Connect signals for children not having a '_yate_autoconnect' property set to false.
     * Install event filter for children with '_yate_filterevents' property set to true.
     * @param parent Parent widget
     * @param name Widget name
     * @param ui UI file to load
     * @return QWidget pointer or 0
     */
    QWidget* loadWidget(QWidget* parent, const String& name, const String& ui);

    /**
     * Load an item's widget using a given type
     * @param parent Parent widget
     * @param name Widget name
     * @param type Item type
     * @return QWidget pointer or 0
     */
    inline QWidget* loadWidgetType(QWidget* parent, const String& name, const String& type) {
	    QtUIWidgetItemProps* p = getItemProps(type);
	    if (p && p->m_ui)
		return loadWidget(parent,name,p->m_ui);
	    return 0;
	}

    /**
     * Apply a QWidget style sheet. Replace ${name} with widget name in style
     * @param name The widget
     * @param style The style sheet to apply
     */
    void applyWidgetStyle(QWidget* w, const String& style);

    /**
     * Filter key press events. Retrieve an action associated with the key.
     * Check if the object is allowed to process the key.
     * Raise the action
     * @param obj The object
     * @param event QKeyEvent event to process
     * @param filter Filter key or let the object process it
     * @return True if processed, false if no key was filtered
     */
    bool filterKeyEvent(QObject* watched, QKeyEvent* event, bool& filter);

    ObjList m_itemProps;
    QStringList m_saveProps;             // List of properties to be automatically
                                         //  saved/restored when window owning
                                         //  this object is initialized/destroyed
    // Navigation
    String m_prev;                       // Goto previous item action
    String m_next;                       // Goto next item action
    String m_info;                       // Info widget: current index, total ...
    String m_infoFormat;                 // Data to be displayed in info
    String m_title;                      // Current item title widget name
};

/**
 * This class encapsulates a custom QT object
 * @short A custom QT object
 */
class YQT4_API QtCustomObject : public QObject, public QtUIWidget
{
    YCLASS(QtCustomObject,QtUIWidget)
    Q_CLASSINFO("QtCustomObject","Yate")
    Q_OBJECT
public:
    /**
     * Constructor
     * @param name Object's name
     * @param parent Optional parent object
     */
    inline QtCustomObject(const char* name, QObject* parent = 0)
	: QObject(parent), QtUIWidget(name)
	{ setObjectName(name);	}

    /**
     * Retrieve a QObject from this one
     * @return QObject pointer
     */
    virtual QObject* getQObject()
	{ return static_cast<QObject*>(this); }

    /**
     * Parent changed notification
     */
    virtual void parentChanged()
	{}

private:
    QtCustomObject() {}                  // No default constructor
};

/**
 * This class encapsulates a custom QT widget
 * @short A custom QT widget
 */
class YQT4_API QtCustomWidget : public QWidget, public QtUIWidget
{
    YCLASS(QtCustomWidget,QtUIWidget)
    Q_CLASSINFO("QtCustomWidget","Yate")
    Q_OBJECT
public:
    /**
     * Constructor
     * @param name Widget's name
     * @param parent Optional parent widget
     */
    inline QtCustomWidget(const char* name, QWidget* parent = 0)
	: QWidget(parent), QtUIWidget(name)
	{ setObjectName(name);	}

    /**
     * Retrieve a QObject from this one
     * @return QObject pointer
     */
    virtual QObject* getQObject()
	{ return static_cast<QObject*>(this); }

private:
    QtCustomWidget() {}                  // No default constructor
};

/**
 * This class encapsulates a custom QT table
 * @short A custom QT table widget
 */
class YQT4_API QtTable : public QTableWidget, public QtUIWidget
{
    YCLASS(QtTable,QtUIWidget)
    Q_CLASSINFO("QtTable","Yate")
    Q_OBJECT
public:
    /**
     * Constructor
     * @param name Table's name
     * @param parent Optional parent widget
     */
    inline QtTable(const char* name, QWidget* parent = 0)
	: QTableWidget(parent), QtUIWidget(name)
	{ setObjectName(name); }

    /**
     * Retrieve a QObject from this one
     * @return QObject pointer
     */
    virtual QObject* getQObject()
	{ return static_cast<QObject*>(this); }

private:
    QtTable() {}                         // No default constructor
};

/**
 * This class encapsulates a custom QT tree
 * @short A custom QT tree widget
 */
class YQT4_API QtTree : public QTreeWidget, public QtUIWidget
{
    YCLASS(QtTree,QtUIWidget)
    Q_CLASSINFO("QtTree","Yate")
    Q_OBJECT
public:
    /**
     * Constructor
     * @param name Tree's name
     * @param parent Optional parent widget
     */
    inline QtTree(const char* name, QWidget* parent = 0)
	: QTreeWidget(parent), QtUIWidget(name)
	{ setObjectName(name); }

    /**
     * Retrieve a QObject from this one
     * @return QObject pointer
     */
    virtual QObject* getQObject()
	{ return static_cast<QObject*>(this); }

private:
    QtTree() {}                          // No default constructor
};

/**
 * QT specific sound
 * @short A QT client sound
 */
class YQT4_API QtSound : public ClientSound
{
    YCLASS(QtSound,ClientSound)
public:
    /**
     * Constructor
     * @param name The name of this object
     * @param file The file to play (should contain the whole path and the file name)
     * @param device Optional device used to play the file. Set to 0 to use the default one
     */
    inline QtSound(const char* name, const char* file, const char* device = 0)
	: ClientSound(name,file,device), m_sound(0)
	{ m_native = true; }

protected:
    virtual bool doStart();
    virtual void doStop();

private:
    QSound* m_sound;
};

}; // namespace TelEngine

#endif // __QT4CLIENT_H

/* vi: set ts=8 sw=4 sts=4 noet: */
