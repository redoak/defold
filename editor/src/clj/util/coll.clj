;; Copyright 2020-2023 The Defold Foundation
;; Copyright 2014-2020 King
;; Copyright 2009-2014 Ragnar Svensson, Christian Murray
;; Licensed under the Defold License version 1.0 (the "License"); you may not use
;; this file except in compliance with the License.
;; 
;; You may obtain a copy of the License, together with FAQs at
;; https://www.defold.com/license
;; 
;; Unless required by applicable law or agreed to in writing, software distributed
;; under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
;; CONDITIONS OF ANY KIND, either express or implied. See the License for the
;; specific language governing permissions and limitations under the License.

(ns util.coll
  (:refer-clojure :exclude [bounded-count empty?])
  (:import [clojure.lang MapEntry]))

(set! *warn-on-reflection* true)
(set! *unchecked-math* :warn-on-boxed)

(defn pair
  "Constructs a two-element collection that implements IPersistentVector from
  the supplied arguments."
  [a b]
  (MapEntry. a b))

(defn flipped-pair
  "Constructs a two-element collection that implements IPersistentVector from
  the reversed arguments."
  [a b]
  (MapEntry. b a))

(defn bounded-count
  "Like core.bounded-count, but tags the return value and limit argument as long
  values to avoid boxed math or reflection. If coll is counted? returns its
  count, else will count at most the first n elements of coll using its seq."
  ^long [^long n coll]
  (if (counted? coll)
    (count coll)
    (loop [i 0
           s (seq coll)]
      (if (and s (< i n))
        (recur (inc i) (next s))
        i))))

(defn resize
  "Returns a collection of the desired size, retaining elements from coll up to
  the desired size. If coll does not have enough elements, fill the subsequent
  slots in the output collection with the supplied fill-value."
  [coll ^long new-count fill-value]
  {:pre [(sequential? coll)
         (not (neg? new-count))]}
  (let [old-count (count coll)]
    (cond
      (< new-count old-count)
      (into (empty coll)
            (take new-count)
            coll)

      (> new-count old-count)
      (into coll
            (repeat (- new-count old-count)
                    fill-value))

      :else
      coll)))

(defn empty?
  "Like core.empty?, but avoids generating garbage for counted collections."
  [coll]
  (cond
    (counted? coll)
    (zero? (count coll))

    (nil? coll)
    true

    :else
    (not (seq coll))))

(defn mapcat-indexed
  "Returns the result of applying concat to the result of applying map-indexed
  to f and coll. Thus function f should return a collection. Returns a
  transducer when no collection is provided."
  ([f] (comp (map-indexed f) cat))
  ([f coll] (sequence (mapcat-indexed f) coll)))

(into []
      (mapcat-indexed (fn [index value]
                  [[:one index value]
                   [:two index value]]))
      [:a :b :c :d])

(defn search
  "Traverses the supplied collection hierarchy recursively, applying the
  specified match-fn to every non-collection value. Records are considered
  values, not collections. Returns a sequence of all non-nil results returned by
  the match-fn."
  [coll match-fn]
  (cond
    (record? coll)
    (when-some [match (match-fn coll)]
      [match])

    (map? coll)
    (eduction
      (mapcat
        (fn [entry]
          (search (val entry) match-fn)))
      coll)

    (coll? coll)
    (eduction
      (mapcat
        (fn [value]
          (search value match-fn)))
      coll)

    :else
    (when-some [match (match-fn coll)]
      [match])))

(defn search-with-path
  "Traverses the supplied collection hierarchy recursively, applying the
  specified match-fn to every non-collection value. Records are considered
  values, not collections. Returns a sequence of pairs of every non-nil result
  returned by the match-fn and the path to the match from the root of the
  collection. Path tokens will be conjoined onto the supplied init-path at each
  level and the resulting path will be part of the matching result pair."
  [coll init-path match-fn]
  (cond
    (record? coll)
    (when-some [match (match-fn coll)]
      [(pair match init-path)])

    (map? coll)
    (eduction
      (mapcat
        (fn [[key value]]
          (search-with-path value (conj init-path key) match-fn)))
      coll)

    (coll? coll)
    (eduction
      (mapcat-indexed
        (fn [index value]
          (search-with-path value (conj init-path index) match-fn)))
      coll)

    :else
    (when-some [match (match-fn coll)]
      [(pair match init-path)])))
