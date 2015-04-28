(ns editor.app-view
  (:require [dynamo.graph :as g]
            [dynamo.types :as t]
            [editor.jfx :as jfx]
            [editor.menu :as menu]
            [editor.project :as project]
            [editor.handler :as handler]
            [editor.ui :as ui]
            [editor.workspace :as workspace])
  (:import [com.defold.editor Start]
           [com.jogamp.opengl.util.awt Screenshot]
           [java.awt Desktop]
           [javafx.animation AnimationTimer]
           [javafx.application Platform]
           [javafx.beans.value ChangeListener]
           [javafx.collections FXCollections ObservableList ListChangeListener]
           [javafx.embed.swing SwingFXUtils]
           [javafx.event ActionEvent EventHandler]
           [javafx.fxml FXMLLoader]
           [javafx.geometry Insets]
           [javafx.scene Scene Node Parent]
           [javafx.scene.control Button ColorPicker Label TextField TitledPane TextArea TreeItem TreeCell Menu MenuItem MenuBar TabPane Tab ProgressBar]
           [javafx.scene.image Image ImageView WritableImage PixelWriter]
           [javafx.scene.input MouseEvent]
           [javafx.scene.layout AnchorPane GridPane StackPane HBox Priority]
           [javafx.scene.paint Color]
           [javafx.stage Stage FileChooser]
           [javafx.util Callback]
           [java.io File]
           [java.nio.file Paths]
           [java.util.prefs Preferences]
           [javax.media.opengl GL GL2 GLContext GLProfile GLDrawableFactory GLCapabilities]))

(def static-menu [{:label "File" :children []}
                  {:label "Edit" :children []}
                  {:label "Help" :children [{:label "About Defold"
                                             :handler-fn (fn [event] (prn "ABOUT!"))}]}])

(g/defnk produce-menu-bar [main-menu menu-bar]
  (.setAll (.getMenus menu-bar) (menu/make-menus main-menu))
  menu-bar)

(g/defnode AppView
  (property stage Stage)
  (property menu-bar MenuBar)
  (property tab-pane TabPane)
  (property refresh-timer AnimationTimer)
  (property auto-pulls t/Any)
  (property active-tool t/Any)

  (input main-menus [t/Any])
  (input outline t/Any)

  (output main-menu t/Any :cached (g/fnk [main-menus] (reduce menu/merge-menus static-menu main-menus)))
  (output menu-bar MenuBar :cached produce-menu-bar)
  (output active-outline t/Any :cached (g/fnk [outline] outline))
  (output active-resource t/Any (g/fnk [tab-pane] (when-let [tab (-> tab-pane (.getSelectionModel) (.getSelectedItem))] (:resource (.getUserData tab)))))
  (output open-resources t/Any (g/fnk [tab-pane] (map (fn [tab] (:resource (.getUserData tab))) (.getTabs tab-pane))))

  (trigger stop-animation :deleted (fn [tx graph self label trigger]
                                     (.stop ^AnimationTimer (:refresh-timer self)))))

(defn- invalidate [node label]
  (g/invalidate! [[(g/node-id node) label]]))

(defn- disconnect-sources [target-node target-label]
  (for [[source-node source-label] (g/sources-of (g/now) target-node target-label)]
    (g/disconnect source-node source-label target-node target-label)))

(defn- replace-connection [source-node source-label target-node target-label]
  (concat
    (disconnect-sources target-node target-label)
    (if (and source-node (contains? (g/outputs source-node) source-label))
      (g/connect source-node source-label target-node target-label)
      [])))

(defn- on-selected-tab-changed [app-view resource-node]
  (g/transact
    (replace-connection resource-node :outline app-view :outline))
  (invalidate app-view :active-resource))

(defn- on-tabs-changed [app-view]
  (invalidate app-view :open-resources))

(handler/defhandler move-tool :move-tool
  (visible? [app-view] true)
  (enabled? [app-view] true)
  (run [app-view] (g/transact (g/set-property app-view :active-tool :move-tool)))
  (state [app-view] (= (:active-tool (g/refresh app-view)) :move-tool)))

(handler/defhandler scale-tool :scale-tool
  (visible? [app-view] true)
  (enabled? [app-view] true)
  (run [app-view] (g/transact (g/set-property app-view :active-tool :scale-tool)))
  (state [app-view]  (= (:active-tool (g/refresh app-view)) :scale-tool)))

(handler/defhandler rotate-tool :rotate-tool
  (visible? [app-view] true)
  (enabled? [app-view] true)
  (run [app-view] (g/transact (g/set-property app-view :active-tool :rotate-tool)))
  (state [app-view]  (= (:active-tool (g/refresh app-view)) :rotate-tool)))

(defn make-app-view [graph stage menu-bar tab-pane]
  (.setUseSystemMenuBar menu-bar true)
  (.setTitle stage "Defold Editor 2.0!")
  (let [app-view (first (g/tx-nodes-added (g/transact (g/make-node graph AppView :stage stage :menu-bar menu-bar :tab-pane tab-pane :active-tool :move-tool))))
        binder (ui/make-ui-binder (.getScene stage))
        arg-map {:app-view app-view}]
    (ui/bind-toolbar binder "#toolbar" arg-map)
    (-> tab-pane
      (.getSelectionModel)
      (.selectedItemProperty)
      (.addListener
        (reify ChangeListener
          (changed [this observable old-val new-val]
            (on-selected-tab-changed app-view (when new-val (.getUserData new-val)))))))
    (-> tab-pane
      (.getTabs)
      (.addListener
        (reify ListChangeListener
          (onChanged [this change]
            (on-tabs-changed app-view)))))
    (let [refresh-timer (proxy [AnimationTimer] []
                          (handle [now]
                            ; TODO: Not invoke this function every frame...
                            (ui/refresh binder arg-map)
                            (let [auto-pulls (:auto-pulls (g/refresh app-view))]
                              (doseq [[node label] auto-pulls]
                                (g/node-value node label)))))]
      (g/transact
        (concat
          (g/set-property app-view :refresh-timer refresh-timer)
          (g/set-property app-view :auto-pulls [[app-view :menu-bar]])))
      (.start refresh-timer))
    app-view))

(defn open-resource [app-view workspace project resource]
  (let [resource-node (project/get-resource-node project resource)
        resource-type (project/get-resource-type resource-node)
        view-type (or (first (:view-types resource-type)) (workspace/get-view-type workspace :text))]
    (if-let [make-view-fn (:make-view-fn view-type)]
      (let [tab-pane   (:tab-pane app-view)
            parent     (AnchorPane.)
            tab        (doto (Tab. (workspace/resource-name resource)) (.setContent parent) (.setUserData resource-node))
            tabs       (doto (.getTabs tab-pane) (.add tab))
            view-graph (g/make-graph! :history false :volatility 2)
            view       (make-view-fn view-graph parent resource-node (assoc ((:id view-type) (:view-opts resource-type)) :select-fn (fn [selection op-seq] (project/select project selection op-seq))))]
        (g/transact (g/connect project :selection view :selection))
        (.setGraphic tab (jfx/get-image-view (:icon resource-type "icons/cog.png")))
        (.setOnClosed tab (ui/event-handler event (g/delete-graph view-graph)))
        (.select (.getSelectionModel tab-pane) tab))
      (.open (Desktop/getDesktop) (File. (workspace/abs-path resource))))))
