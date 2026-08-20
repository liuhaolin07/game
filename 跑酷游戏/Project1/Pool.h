// ============================================================
// Pool.h — 泛型对象池模板类
// 功能: 提供增删改查等通用容器操作，
//       演示 C++ 模板、STL 容器、迭代器、STL算法等核心知识点
// 使用: Pool<Obstacle> pool;  pool.Add(obstacle);
// ============================================================
#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <iostream>

/**
 * 泛型对象池模板类
 * 封装 std::vector 提供更语义化的增删改查接口
 * @tparam T 池中存储的对象类型
 */
template<typename T>
class Pool {
public:
    // 类型别名（方便外部使用迭代器）
    using Iterator = typename std::vector<T>::iterator;
    using ConstIterator = typename std::vector<T>::const_iterator;

    // 默认构造和析构
    Pool() = default;
    ~Pool() = default;

    // ========== 增 ==========
    /** 在末尾添加对象 */
    void Add(const T& obj) {
        items.push_back(obj);
    }

    /** 在指定位置前插入 */
    void Insert(Iterator it, const T& obj) {
        items.insert(it, obj);
    }

    // ========== 删 ==========
    /** 移除指定对象（通过==比较） */
    bool Remove(const T& obj) {
        auto it = std::find(items.begin(), items.end(), obj);
        if (it != items.end()) {
            items.erase(it);
            return true;
        }
        return false;
    }

    /** 按条件批量移除（传入谓词函数） */
    int RemoveIf(std::function<bool(const T&)> predicate) {
        auto before = items.size();
        items.erase(std::remove_if(items.begin(), items.end(), predicate), items.end());
        return static_cast<int>(before - items.size());
    }

    /** 清空所有对象 */
    void Clear() {
        items.clear();
    }

    // ========== 改 ==========
    /** 对每个元素执行操作 */
    void ForEach(std::function<void(T&)> func) {
        for (auto& item : items) {
            func(item);
        }
    }

    // ========== 查 ==========
    /** 查找第一个匹配的元素（返回迭代器） */
    Iterator Find(std::function<bool(const T&)> predicate) {
        return std::find_if(items.begin(), items.end(), predicate);
    }

    ConstIterator Find(std::function<bool(const T&)> predicate) const {
        return std::find_if(items.begin(), items.end(), predicate);
    }

    /** 统计匹配条件的元素数量 */
    int CountIf(std::function<bool(const T&)> predicate) const {
        return static_cast<int>(std::count_if(items.begin(), items.end(), predicate));
    }

    /** 获取大小/判空 */
    size_t Size() const { return items.size(); }
    bool Empty() const { return items.empty(); }

    // ========== 下标访问 ==========
    T& operator[](size_t index) { return items[index]; }
    const T& operator[](size_t index) const { return items[index]; }

    // ========== 迭代器（支持范围for循环） ==========
    Iterator begin() { return items.begin(); }
    Iterator end() { return items.end(); }
    ConstIterator begin() const { return items.begin(); }
    ConstIterator end() const { return items.end(); }

    // ========== 运算符重载 ==========
    /** += 添加对象到池中 */
    Pool<T>& operator+=(const T& obj) {
        Add(obj);
        return *this;
    }

    /** -= 从池中移除对象 */
    Pool<T>& operator-=(const T& obj) {
        Remove(obj);
        return *this;
    }

    /** == 比较两个池的大小 */
    bool operator==(const Pool<T>& other) const {
        return items.size() == other.items.size();
    }

    bool operator!=(const Pool<T>& other) const {
        return !(*this == other);
    }

    /** << 输出池信息到流 */
    friend std::ostream& operator<<(std::ostream& os, const Pool<T>& pool) {
        os << "Pool(size=" << pool.items.size() << ")";
        return os;
    }

private:
    std::vector<T> items;   // 内部使用STL vector存储对象
};
