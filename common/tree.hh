/* 

   $Id: tree.hh,v 1.1 2003/01/28 04:17:02 mvogt Exp $

   STL-like templated tree class.
   Copyright (C) 2001  Kasper Peeters <k.peeters@damtp.cam.ac.uk>

   See 
  
      http://www.damtp.cam.ac.uk/user/kp229/tree/

   for more information and documentation. See the Changelog file for
   other credits.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
   TODO: - operator++ should not return an iterator_base. Change this and
           then remove the various sibling_iterator(++ch) constructions.
         - if a range uses const iter_base& as end iterator, things will
           inevitably go wrong, because upcast from iter_base to a non-sibling_iter
           is incorrect. This upcast should be removed (and then all illegal uses
           as previously in 'equal' will be flagged by the compiler). This requires
           new copy constructors though.
*/

#ifndef tree_hh_
#define tree_hh_

#include <cassert>
#include <memory>
#include <stdexcept>
#include <iterator>
#include <set>

// HP-style construct/destroy have gone from the standard,
// so here is a copy.

template <class T1, class T2>
inline void constructor(T1* p, T2& val) 
   {
   new ((void *) p) T1(val);
   }

template <class T1>
inline void constructor(T1* p) 
   {
   new ((void *) p) T1;
   }

template <class T1>
inline void destructor(T1* p)
   {
   p->~T1();
   }


template<class T>
struct tree_node_ {
      tree_node_<T> *parent;
      tree_node_<T> *first_child, *last_child;
      tree_node_<T> *prev_sibling, *next_sibling;
      T data;
};

template <class T, class tree_node_allocator = std::allocator<tree_node_<T> > >
class tree {
   public:
      typedef tree_node_<T> tree_node;
      typedef T value_type;

      class iterator_base;
      class pre_order_iterator;
      class post_order_iterator;
      class sibling_iterator;

      tree();
      tree(const T&);
      tree(const iterator_base&);
      tree(const tree<T, tree_node_allocator>&);
      ~tree();
      void operator=(const tree<T, tree_node_allocator>&);

      class iterator_base {
         public:
            typedef T                               value_type;
            typedef T*                              pointer;
            typedef T&                              reference;
            typedef size_t                          size_type;
            typedef ptrdiff_t                       difference_type;
            typedef std::bidirectional_iterator_tag iterator_category;

            iterator_base();
            iterator_base(tree_node *);

            virtual iterator_base&  operator++()=0;
            virtual iterator_base&  operator--()=0;
            virtual iterator_base&  operator+=(unsigned int)=0;
            virtual iterator_base&  operator-=(unsigned int)=0;

            T&             operator*() const;
            T*             operator->() const;

            void         skip_children(); // do not iterate over children of this node
            unsigned int number_of_children() const;

            sibling_iterator begin() const;
            sibling_iterator end() const;

            tree_node *node;
         protected:
            bool skip_current_children_;
      };

      class pre_order_iterator : public iterator_base { 
         public:
            pre_order_iterator();
            pre_order_iterator(tree_node *);
            pre_order_iterator(const iterator_base&);
            pre_order_iterator(const sibling_iterator&);

            bool    operator==(const pre_order_iterator&) const;
            bool    operator!=(const pre_order_iterator&) const;
            virtual iterator_base&  operator++();
            virtual iterator_base&  operator--();
            virtual iterator_base&  operator+=(unsigned int);
            virtual iterator_base&  operator-=(unsigned int);
      };

      class post_order_iterator : public iterator_base {
         public:
            post_order_iterator();
            post_order_iterator(tree_node *);
            post_order_iterator(const iterator_base&);
            post_order_iterator(const sibling_iterator&);

            bool    operator==(const post_order_iterator&) const;
            bool    operator!=(const post_order_iterator&) const;
            virtual iterator_base&  operator++();
            virtual iterator_base&  operator--();
            virtual iterator_base&  operator+=(unsigned int);
            virtual iterator_base&  operator-=(unsigned int);

            void descend_all();
      };

      typedef pre_order_iterator iterator;

      class sibling_iterator : public iterator_base {
         public:
            sibling_iterator();
            sibling_iterator(tree_node *);
            sibling_iterator(const sibling_iterator&);
            sibling_iterator(const iterator_base&);

            bool    operator==(const sibling_iterator&) const;
            bool    operator!=(const sibling_iterator&) const;
            virtual iterator_base&  operator++();
            virtual iterator_base&  operator--();
            virtual iterator_base&  operator+=(unsigned int);
            virtual iterator_base&  operator-=(unsigned int);

            tree_node *range_first() const;
            tree_node *range_last() const;
            tree_node *parent_;
         private:
            void set_parent_();
      };

      // begin/end of tree
      pre_order_iterator  begin() const;
      pre_order_iterator  end() const;
      post_order_iterator begin_post() const;
      post_order_iterator end_post() const;
      // begin/end of children of node
      sibling_iterator begin(const iterator_base&) const;
      sibling_iterator end(const iterator_base&) const;
      template<typename iter> iter parent(iter) const;
      sibling_iterator previous_sibling(const iterator_base&) const;
      sibling_iterator next_sibling(const iterator_base&) const;

      void     clear();
      // erase element at position pointed to by iterator, increment iterator
      template<typename iter> iter erase(iter);
      // erase all children of the node pointed to by iterator
      void     erase_children(const iterator_base&);

      // insert node as last child of node pointed to by position (first one inserts empty node)
      template<typename iter> iter append_child(iter position); 
      template<typename iter> iter append_child(iter position, const T& x);
      // the following two append nodes plus their children
      template<typename iter> iter append_child(iter position, iter other_position);
      template<typename iter> iter append_children(iter position, sibling_iterator from, sibling_iterator to);

      // short-hand to insert topmost node in otherwise empty tree
      pre_order_iterator set_head(const T& x);
      // insert node as previous sibling of node pointed to by position
      template<typename iter> iter insert(iter position, const T& x);
      // specialisation: insert node as previous sibling of node pointed to by position
      //pre_order_iterator insert(sibling_iterator position, const T& x);
      sibling_iterator insert(sibling_iterator position, const T& x);
      // insert node (with children) pointed to by subtree as previous sibling of node pointed to by position
      template<typename iter> iter insert_subtree(iter position, const iterator_base& subtree);
      // insert node as next sibling of node pointed to by position
      template<typename iter> iter insert_after(iter position, const T& x);

      // replace node at 'position' with other node (keeping same children); 'position' becomes invalid.
      template<typename iter> iter replace(iter position, const T& x);
      // replace node at 'position' with subtree starting at 'from' (do not erase subtree at 'from'); see above.
      template<typename iter> iter replace(iter position, const iterator_base& from);
      // replace string of siblings (plus their children) with copy of a new string (with children); see above
      sibling_iterator replace(sibling_iterator orig_begin, sibling_iterator orig_end, 
                               sibling_iterator new_begin,  sibling_iterator new_end); 

      // move all children of node at 'position' to be siblings, returns position
      template<typename iter> iter flatten(iter position);
      // move nodes in range to be children of 'position'
      template<typename iter> iter reparent(iter position, sibling_iterator begin, sibling_iterator end);
      // ditto, the range being all children of the 'from' node
      template<typename iter> iter reparent(iter position, iter from);
      // merge with other tree, creating new branches and leaves only if they are not already present
      void     merge(sibling_iterator, sibling_iterator, sibling_iterator, sibling_iterator, 
                     bool duplicate_leaves=false);
      // sort (std::sort only moves values of nodes, this one moves children as well)
      void     sort(sibling_iterator from, sibling_iterator to, bool deep=false);
      template<class StrictWeakOrdering>
      void     sort(sibling_iterator from, sibling_iterator to, StrictWeakOrdering comp, bool deep=false);
      // compare two ranges of nodes (compares nodes as well as tree structure)
      template<typename iter>
      bool     equal(const iter& one, const iter& two, const iter& three) const;
      template<typename iter, class BinaryPredicate>
      bool     equal(const iter& one, const iter& two, const iter& three, BinaryPredicate) const;
      // extract a new tree formed by the range of siblings plus all their children
      tree     subtree(sibling_iterator from, sibling_iterator to) const;
      void     subtree(tree&, sibling_iterator from, sibling_iterator to) const;
      // exchange the node (plus subtree) with its sibling node (do nothing if no sibling present)
      void     swap(sibling_iterator it);
      
      // count the total number of nodes
      int      size() const;
      // compute the depth to the root
      int      depth(const iterator_base&) const;
      // count the number of children of node at position
      unsigned int number_of_children(const iterator_base&) const;
      // count the number of 'next' siblings of node at iterator
      unsigned int number_of_siblings(const iterator_base&) const;
      // determine whether node at position is in the subtrees with root in the range
      bool     is_in_subtree(const iterator_base& position, const iterator_base& begin, 
                             const iterator_base& end) const;
      // determine whether the iterator is an 'end' iterator and thus not actually
      // pointing to a node
      // DEPRECATE: this causes more trouble than it's worth.
      bool     is_valid(const iterator_base&) const;

      // determine the index of a node in the range of siblings to which it belongs.
      unsigned int index(sibling_iterator it) const;
      // inverse of 'index': return the n-th child of the node at position
      T&       child(const iterator_base& position, unsigned int) const;

   private:
      tree_node_allocator alloc_;
      tree_node *head;    // head is always a dummy; if an iterator points to head it is invalid
      void head_initialise_();
      void copy_(const tree<T, tree_node_allocator>& other);
      template<class StrictWeakOrdering>
      class compare_nodes {
         public:
            bool operator()(const tree_node*, const tree_node *);
      };
};



// Tree

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::tree() 
   {
   head_initialise_();
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::tree(const T& x) 
   {
   head_initialise_();
   set_head(x);
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::tree(const iterator_base& other)
   {
   head_initialise_();
   set_head((*other));
   replace(begin(), other);
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::~tree()
   {
   clear();
   alloc_.deallocate(head,1);
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::head_initialise_() 
   { 
   head = alloc_.allocate(1,0); // MSVC does not have default second argument 

   head->parent=0;
   head->first_child=0;
   head->last_child=0;
   head->prev_sibling=head;
   head->next_sibling=head;
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::operator=(const tree<T, tree_node_allocator>& other)
   {
   copy_(other);
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::tree(const tree<T, tree_node_allocator>& other)
   {
   head_initialise_();
   copy_(other);
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::copy_(const tree<T, tree_node_allocator>& other) 
   {
   clear();
   pre_order_iterator it=other.begin(), to=begin();
   while(it!=other.end()) {
      to=insert(to, (*it));
      it.skip_children();
      ++it;
      }
   to=begin();
   it=other.begin();
   while(it!=other.end()) {
      to=replace(to, it);
      to.skip_children();
      it.skip_children();
      ++to;
      ++it;
      }
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::clear()
   {
   if(head)
      while(head->next_sibling!=head)
         erase(pre_order_iterator(head->next_sibling));
   }

template<class T, class tree_node_allocator> 
void tree<T, tree_node_allocator>::erase_children(const iterator_base& it)
   {
   tree_node *cur=it.node->first_child;
   tree_node *prev=0;

   while(cur!=0) {
      prev=cur;
      cur=cur->next_sibling;
      erase_children(pre_order_iterator(prev));
      destructor(&prev->data);
      alloc_.deallocate(prev,1);
      }
   it.node->first_child=0;
   it.node->last_child=0;
   }

template<class T, class tree_node_allocator> 
template<class iter>
iter tree<T, tree_node_allocator>::erase(iter it)
   {
   tree_node *cur=it.node;
   assert(cur!=head);
   iter ret=it;
   ret.skip_children();
   ++ret;
   erase_children(it);
   if(cur->prev_sibling==0) {
      cur->parent->first_child=cur->next_sibling;
      }
   else {
      cur->prev_sibling->next_sibling=cur->next_sibling;
      }
   if(cur->next_sibling==0) {
      cur->parent->last_child=cur->prev_sibling;
      }
   else {
      cur->next_sibling->prev_sibling=cur->prev_sibling;
      }

   destructor(&cur->data);
   alloc_.deallocate(cur,1);
   return ret;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::pre_order_iterator tree<T, tree_node_allocator>::begin() const
   {
   return pre_order_iterator(head->next_sibling);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::pre_order_iterator tree<T, tree_node_allocator>::end() const
   {
   return pre_order_iterator(head);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::post_order_iterator tree<T, tree_node_allocator>::begin_post() const
   {
   tree_node *tmp=head->next_sibling;
   if(tmp!=head) {
      while(tmp->first_child)
         tmp=tmp->first_child;
      }
   return post_order_iterator(tmp);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::post_order_iterator tree<T, tree_node_allocator>::end_post() const
   {
   return post_order_iterator(head);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::begin(const iterator_base& pos) const
   {
   if(pos.node->first_child==0) {
      return end(pos);
      }
   return pos.node->first_child;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::end(const iterator_base& pos) const
   {
   sibling_iterator ret(0);
   ret.parent_=pos.node;
   return ret;
   }

template <class T, class tree_node_allocator>
template <typename iter>
iter tree<T, tree_node_allocator>::parent(iter position) const
   {
   assert(position.node!=0);
   return iter(position.node->parent);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::previous_sibling(const iterator_base& position) const
   {
   assert(position.node!=0);
   return sibling_iterator(position.node->prev_sibling);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::next_sibling(const iterator_base& position) const
   {
   assert(position.node!=0);
   if(position.node->next_sibling==0)
      return end(pre_order_iterator(position.node->parent));
   else
      return sibling_iterator(position.node->next_sibling);
   }

template <class T, class tree_node_allocator>
template <typename iter>
iter tree<T, tree_node_allocator>::append_child(iter position)
   {
   assert(position.node!=head);

   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data);
   tmp->first_child=0;
   tmp->last_child=0;

   tmp->parent=position.node;
   if(position.node->last_child!=0) {
      position.node->last_child->next_sibling=tmp;
      }
   else {
      position.node->first_child=tmp;
      }
   tmp->prev_sibling=position.node->last_child;
   position.node->last_child=tmp;
   tmp->next_sibling=0;
   return tmp;
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::append_child(iter position, const T& x)
   {
   // If your program fails here you probably used 'append_child' to add the top
   // node to an empty tree. From version 1.45 the top element should be added
   // using 'insert'. See the documentation for further information, and sorry about
   // the API change.
   assert(position.node!=head);

   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data, x);
   tmp->first_child=0;
   tmp->last_child=0;

   tmp->parent=position.node;
   if(position.node->last_child!=0) {
      position.node->last_child->next_sibling=tmp;
      }
   else {
      position.node->first_child=tmp;
      }
   tmp->prev_sibling=position.node->last_child;
   position.node->last_child=tmp;
   tmp->next_sibling=0;
   return tmp;
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::append_child(iter position, iter other)
   {
   assert(position.node!=head);

   sibling_iterator aargh=append_child(position, value_type());
   return replace(aargh, ++aargh, other, ++sibling_iterator(other));
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::append_children(iter position, sibling_iterator from, sibling_iterator to)
   {
   iter ret=from;

   while(from!=to) {
      insert_subtree(position.end(), from);
      ++from;
      }
   return ret;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::pre_order_iterator tree<T, tree_node_allocator>::set_head(const T& x)
   {
   assert(begin()==end());
   return insert(begin(), x);
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::insert(iter position, const T& x)
   {
   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data, x);
   tmp->first_child=0;
   tmp->last_child=0;

   tmp->parent=position.node->parent;
   tmp->next_sibling=position.node;
   tmp->prev_sibling=position.node->prev_sibling;
   position.node->prev_sibling=tmp;

   if(tmp->prev_sibling==0)
      tmp->parent->first_child=tmp;
   else
      tmp->prev_sibling->next_sibling=tmp;
   return tmp;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::insert(sibling_iterator position, const T& x)
   {
   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data, x);
   tmp->first_child=0;
   tmp->last_child=0;

   tmp->next_sibling=position.node;
   if(position.node==0) { // iterator points to end of a subtree
      tmp->parent=position.parent_;
      tmp->prev_sibling=position.range_last();
      }
   else {
      tmp->parent=position.node->parent;
      tmp->prev_sibling=position.node->prev_sibling;
      position.node->prev_sibling=tmp;
      }

   if(tmp->prev_sibling==0)
      tmp->parent->first_child=tmp;
   else
      tmp->prev_sibling->next_sibling=tmp;
   return tmp;
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::insert_after(iter position, const T& x)
   {
   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data, x);
   tmp->first_child=0;
   tmp->last_child=0;

   tmp->parent=position.node->parent;
   tmp->prev_sibling=position.node;
   tmp->next_sibling=position.node->next_sibling;
   position.node->next_sibling=tmp;

   if(tmp->next_sibling==0) {
      tmp->parent->last_child=tmp;
      }
   else {
      tmp->next_sibling->prev_sibling=tmp;
      }
   return tmp;
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::insert_subtree(iter position, const iterator_base& subtree)
   {
   // insert dummy
   iter it=insert(position, value_type());
   // replace dummy with subtree
   return replace(it, subtree);
   }

// template <class T, class tree_node_allocator>
// template <class iter>
// iter tree<T, tree_node_allocator>::insert_subtree(sibling_iterator position, iter subtree)
//    {
//    // insert dummy
//    iter it(insert(position, value_type()));
//    // replace dummy with subtree
//    return replace(it, subtree);
//    }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::replace(iter position, const T& x)
   {
   destructor(&position.node->data);
   constructor(&position.node->data, x);
   return position;
   }

template <class T, class tree_node_allocator>
template <class iter>
iter tree<T, tree_node_allocator>::replace(iter position, const iterator_base& from)
   {
   assert(position.node!=head);

   tree_node *current_from=from.node;
   tree_node *start_from=from.node;
   tree_node *last=from.node->next_sibling;
   tree_node *current_to  =position.node;

   // replace the node at position with head of the replacement tree at from
   erase_children(position);  
   tree_node* tmp = alloc_.allocate(1,0);
   constructor(&tmp->data, (*from));
   tmp->first_child=0;
   tmp->last_child=0;
   if(current_to->prev_sibling==0) {
      current_to->parent->first_child=tmp;
      }
   else {
      current_to->prev_sibling->next_sibling=tmp;
      }
   tmp->prev_sibling=current_to->prev_sibling;
   if(current_to->next_sibling==0) {
      current_to->parent->last_child=tmp;
      }
   else {
      current_to->next_sibling->prev_sibling=tmp;
      }
   tmp->next_sibling=current_to->next_sibling;
   tmp->parent=current_to->parent;
   destructor(&current_to->data);
   alloc_.deallocate(current_to,1);
   current_to=tmp;

   pre_order_iterator toit=tmp;

   // copy all children
   do {
      assert(current_from!=0);
      if(current_from->first_child != 0) {
         current_from=current_from->first_child;
         toit=append_child(toit, current_from->data);
         }
      else {
         while(current_from->next_sibling==0 && current_from!=start_from) {
            current_from=current_from->parent;
            toit=parent(toit);
            assert(current_from!=0);
            }
         current_from=current_from->next_sibling;
         if(current_from!=last) {
            toit=append_child(parent(toit), current_from->data);
            }
         }
      } while(current_from!=last);

   return current_to;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::replace(
   sibling_iterator orig_begin, 
   sibling_iterator orig_end, 
   sibling_iterator new_begin, 
   sibling_iterator new_end)
   {
   tree_node *orig_first=orig_begin.node;
   tree_node *new_first=new_begin.node;
   tree_node *orig_last=orig_first;
   while(sibling_iterator(++orig_begin)!=orig_end)
      orig_last=orig_last->next_sibling;
   tree_node *new_last=new_first;
   while(sibling_iterator(++new_begin)!=new_end)
      new_last=new_last->next_sibling;

   // insert all siblings in new_first..new_last before orig_first
   bool first=true;
   pre_order_iterator ret;
   while(1==1) {
      pre_order_iterator tt=insert_subtree(pre_order_iterator(orig_first), pre_order_iterator(new_first));
      if(first) {
         ret=tt;
         first=false;
         }
      if(new_first==new_last)
         break;
      new_first=new_first->next_sibling;
      }

   // erase old range of siblings
   bool last=false;
   tree_node *next=orig_first;
   while(1==1) {
      if(next==orig_last) 
         last=true;
      next=next->next_sibling;
      erase((pre_order_iterator)orig_first);
      if(last) 
         break;
      orig_first=next;
      }
   return ret;
   }

template <class T, class tree_node_allocator>
template <typename iter>
iter tree<T, tree_node_allocator>::flatten(iter position)
   {
   if(position.node->first_child==0)
      return position;

   tree_node *tmp=position.node->first_child;
   while(tmp) {
      tmp->parent=position.node->parent;
      tmp=tmp->next_sibling;
      } 
   if(position.node->next_sibling) {
      position.node->last_child->next_sibling=position.node->next_sibling;
      position.node->next_sibling->prev_sibling=position.node->last_child;
      }
   else {
      position.node->parent->last_child=position.node->last_child;
      }
   position.node->next_sibling=position.node->first_child;
   position.node->next_sibling->prev_sibling=position.node;
   position.node->first_child=0;
   position.node->last_child=0;

   return position;
   }


template <class T, class tree_node_allocator>
template <typename iter>
iter tree<T, tree_node_allocator>::reparent(iter position, sibling_iterator begin, sibling_iterator end)
   {
   tree_node *first=begin.node;
   tree_node *last=first;
   if(begin==end) return begin;
   // determine last node
   while(sibling_iterator(++begin)!=end) {
      last=last->next_sibling;
      }
   // move subtree
   if(first->prev_sibling==0) {
      first->parent->first_child=last->next_sibling;
      }
   else {
      first->prev_sibling->next_sibling=last->next_sibling;
      }
   if(last->next_sibling==0) {
      last->parent->last_child=first->prev_sibling;
      }
   else {
      last->next_sibling->prev_sibling=first->prev_sibling;
      }
   if(position.node->first_child==0) {
      position.node->first_child=first;
      position.node->last_child=last;
      first->prev_sibling=0;
      }
   else {
      position.node->last_child->next_sibling=first;
      first->prev_sibling=position.node->last_child;
      position.node->last_child=last;
      }
   last->next_sibling=0;

   tree_node *pos=first;
   while(1==1) {
      pos->parent=position.node;
      if(pos==last) break;
      pos=pos->next_sibling;
      }

   return first;
   }

template <class T, class tree_node_allocator>
template <typename iter>
iter tree<T, tree_node_allocator>::reparent(iter position, iter from)
   {
   if(from.node->first_child==0) return position;
   return reparent(position, from.node->first_child, from.node->last_child);
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::merge(sibling_iterator to1,   sibling_iterator to2,
                                          sibling_iterator from1, sibling_iterator from2,
                                          bool duplicate_leaves)
   {
   sibling_iterator fnd;
   while(from1!=from2) {
      if((fnd=std::find(to1, to2, (*from1))) != to2) { // element found
         if(from1.begin()==from1.end()) { // full depth reached
            if(duplicate_leaves)
               append_child(parent(to1), (*from1));
            }
         else { // descend further
            merge(fnd.begin(), fnd.end(), from1.begin(), from1.end(), duplicate_leaves);
            }
         }
      else { // element missing
         insert_subtree(to2, from1);
         }
      ++from1;
      }
   }

template <class T, class tree_node_allocator>
template <class StrictWeakOrdering> 
bool tree<T, tree_node_allocator>::compare_nodes<StrictWeakOrdering>::operator()(const tree_node *a, 
                                                                                 const tree_node *b)
   {
   static StrictWeakOrdering comp;

   return comp(a->data, b->data);
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::sort(sibling_iterator from, sibling_iterator to, bool deep)
   {
   std::less<T> comp;
   sort(from, to, comp, deep);
   }

template <class T, class tree_node_allocator>
template <class StrictWeakOrdering>
void tree<T, tree_node_allocator>::sort(sibling_iterator from, sibling_iterator to, 
                                        StrictWeakOrdering comp, bool deep)
   {
   if(from==to) return;
   // make list of sorted nodes
   // CHECK: if multiset stores equivalent nodes in the order in which they
   // are inserted, then this routine should be called 'stable_sort'.
   std::multiset<tree_node *, compare_nodes<StrictWeakOrdering> > nodes;
   sibling_iterator it=from, it2=to;
   while(it != to) {
      nodes.insert(it.node);
      ++it;
      }
   // reassemble
   --it2;
   tree_node *prev=from.node->prev_sibling;
   tree_node *next=it2.node->next_sibling;
   typename std::multiset<tree_node *, compare_nodes<StrictWeakOrdering> >::iterator nit=nodes.begin(), eit=nodes.end();
   if(prev==0) {
      (*nit)->parent->first_child=(*nit);
      }
   --eit;
   while(nit!=eit) {
      (*nit)->prev_sibling=prev;
      if(prev)
         prev->next_sibling=(*nit);
      prev=(*nit);
      ++nit;
      }
   if(prev)
      prev->next_sibling=(*eit);
   (*eit)->next_sibling=next;
   (*eit)->prev_sibling=prev;
   if(next==0) {
      (*eit)->parent->last_child=next;
      }

   if(deep) {  // sort the children of each node too
      sibling_iterator bcs(*nodes.begin());
      sibling_iterator ecs(*eit);
      ++ecs;
      while(bcs!=ecs) {
         sort(begin(bcs), end(bcs), comp, deep);
         ++bcs;
         }
      }
   }

template <class T, class tree_node_allocator>
template <typename iter>
bool tree<T, tree_node_allocator>::equal(const iter& one_, const iter& two, const iter& three_) const
   {
   std::equal_to<T> comp;
   return equal(one_, two, three_, comp);
   }

template <class T, class tree_node_allocator>
template <typename iter, class BinaryPredicate>
bool tree<T, tree_node_allocator>::equal(const iter& one_, const iter& two, const iter& three_, BinaryPredicate fun) const
   {
   pre_order_iterator one(one_), three(three_);

   while(one!=two && is_valid(three)) {
      if(!fun(*one,*three))
         return false;
      if(one.number_of_children()!=three.number_of_children()) 
         return false;
      ++one;
      ++three;
      }
   return true;
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator> tree<T, tree_node_allocator>::subtree(sibling_iterator from, sibling_iterator to) const
   {
   tree tmp;
   tmp.set_head(value_type());
   tmp.replace(tmp.begin(), tmp.end(), from, to);
   return tmp;
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::subtree(tree& tmp, sibling_iterator from, sibling_iterator to) const
   {
   tmp.set_head(value_type());
   tmp.replace(tmp.begin(), tmp.end(), from, to);
   }

template <class T, class tree_node_allocator>
int tree<T, tree_node_allocator>::size() const
   {
   int i=0;
   pre_order_iterator it=begin(), eit=end();
   while(it!=eit) {
      ++i;
      ++it;
      }
   return i;
   }

template <class T, class tree_node_allocator>
int tree<T, tree_node_allocator>::depth(const iterator_base& it) const
   {
   tree_node* pos=it.node;
   assert(pos!=0);
   int ret=0;
   while(pos->parent!=0) {
      pos=pos->parent;
      ++ret;
      }
   return ret;
   }

template <class T, class tree_node_allocator>
unsigned int tree<T, tree_node_allocator>::number_of_children(const iterator_base& it) const
   {
   tree_node *pos=it.node->first_child;
   if(pos==0) return 0;
   
   unsigned int ret=1;
//   while(pos!=it.node->last_child) {
//      ++ret;
//      pos=pos->next_sibling;
//      }
   while((pos=pos->next_sibling))
      ++ret;
   return ret;
   }

template <class T, class tree_node_allocator>
unsigned int tree<T, tree_node_allocator>::number_of_siblings(const iterator_base& it) const
   {
   tree_node *pos=it.node;
   unsigned int ret=1;
   while(pos->next_sibling && pos->next_sibling!=head) {
      ++ret;
      pos=pos->next_sibling;
      }
   return ret;
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::swap(sibling_iterator it)
   {
   tree_node *nxt=it.node->next_sibling;
   if(nxt) {
      if(it.node->prev_sibling)
         it.node->prev_sibling->next_sibling=nxt;
      else
         it.node->parent->first_child=nxt;
      nxt->prev_sibling=it.node->prev_sibling;
      tree_node *nxtnxt=nxt->next_sibling;
      if(nxtnxt)
         nxtnxt->prev_sibling=it.node;
      else
         it.node->parent->last_child=it.node;
      nxt->next_sibling=it.node;
      it.node->prev_sibling=nxt;
      it.node->next_sibling=nxtnxt;
      }
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::is_in_subtree(const iterator_base& it, const iterator_base& begin, 
                                                 const iterator_base& end) const
   {
   // FIXME: this should be optimised.
   pre_order_iterator tmp=begin;
   while(tmp!=end) {
      if(tmp==it) return true;
      ++tmp;
      }
   return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::is_valid(const iterator_base& it) const
   {
   if(it.node==0) return false;
   else return true;
   }

template <class T, class tree_node_allocator>
unsigned int tree<T, tree_node_allocator>::index(sibling_iterator it) const
   {
   unsigned int ind=0;
   if(it.node->parent==0) {
      while(it.node->prev_sibling!=head) {
         it.node=it.node->prev_sibling;
         ++ind;
         }
      }
   else {
      while(it.node->prev_sibling!=0) {
         it.node=it.node->prev_sibling;
         ++ind;
         }
      }
   return ind;
   }


template <class T, class tree_node_allocator>
T& tree<T, tree_node_allocator>::child(const iterator_base& it, unsigned int num) const
   {
   tree_node *tmp=it.node->first_child;
   while(num--) {
      assert(tmp!=0);
      tmp=tmp->next_sibling;
      }
   return tmp->data;
   }



// Iterator base

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::iterator_base::iterator_base()
   : node(0), skip_current_children_(false)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::iterator_base::iterator_base(tree_node *tn)
   : node(tn), skip_current_children_(false)
   {
   }

template <class T, class tree_node_allocator>
T& tree<T, tree_node_allocator>::iterator_base::operator*() const
   {
   return node->data;
   }

template <class T, class tree_node_allocator>
T* tree<T, tree_node_allocator>::iterator_base::operator->() const
   {
   return &(node->data);
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::post_order_iterator::operator!=(const post_order_iterator& other) const
   {
   if(other.node!=node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::post_order_iterator::operator==(const post_order_iterator& other) const
   {
   if(other.node==node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::pre_order_iterator::operator!=(const pre_order_iterator& other) const
   {
   if(other.node!=node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::pre_order_iterator::operator==(const pre_order_iterator& other) const
   {
   if(other.node==node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::sibling_iterator::operator!=(const sibling_iterator& other) const
   {
   if(other.node!=node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
bool tree<T, tree_node_allocator>::sibling_iterator::operator==(const sibling_iterator& other) const
   {
   if(other.node==node) return true;
   else return false;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::iterator_base::begin() const
   {
   sibling_iterator ret(node->first_child);
   ret.parent_=node;
   return ret;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::sibling_iterator tree<T, tree_node_allocator>::iterator_base::end() const
   {
   sibling_iterator ret(0);
   ret.parent_=node;
   return ret;
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::iterator_base::skip_children()
   {
   skip_current_children_=true;
   }

template <class T, class tree_node_allocator>
unsigned int tree<T, tree_node_allocator>::iterator_base::number_of_children() const
   {
   tree_node *pos=node->first_child;
   if(pos==0) return 0;
   
   unsigned int ret=1;
   while(pos!=node->last_child) {
      ++ret;
      pos=pos->next_sibling;
      }
   return ret;
   }



// Pre-order iterator

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::pre_order_iterator::pre_order_iterator() 
   : iterator_base(0)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::pre_order_iterator::pre_order_iterator(tree_node *tn)
   : iterator_base(tn)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::pre_order_iterator::pre_order_iterator(const iterator_base &other)
   : iterator_base(other.node)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::pre_order_iterator::pre_order_iterator(const sibling_iterator& other)
   : iterator_base(other.node)
   {
   if(node==0) {
      if(other.range_last()!=0)
         node=other.range_last();
      else 
         node=other.parent_;
      skip_children();
      ++(*this);
      }
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::pre_order_iterator::operator++()
   {
   assert(node!=0);
   if(!skip_current_children_ && node->first_child != 0) {
      node=node->first_child;
      }
   else {
      skip_current_children_=false;
      while(node->next_sibling==0) {
         node=node->parent;
         if(node==0)
            return *this;
         }
      node=node->next_sibling;
      }
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::pre_order_iterator::operator--()
   {
   assert(node!=0);
   if(node->prev_sibling) {
      node=node->prev_sibling;
      while(node->last_child)
         node=node->last_child;
      }
   else {
      node=node->parent;
      if(node==0)
         return *this;
      }
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::pre_order_iterator::operator+=(unsigned int num)
   {
   while(num>0) {
      ++(*this);
      --num;
      }
   return (*this);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::pre_order_iterator::operator-=(unsigned int num)
   {
   while(num>0) {
      --(*this);
      --num;
      }
   return (*this);
   }



// Post-order iterator

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::post_order_iterator::post_order_iterator() 
   : iterator_base(0)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::post_order_iterator::post_order_iterator(tree_node *tn)
   : iterator_base(tn)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::post_order_iterator::post_order_iterator(const iterator_base &other)
   : iterator_base(other.node)
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::post_order_iterator::post_order_iterator(const sibling_iterator& other)
   : iterator_base(other.node)
   {
   if(node==0) {
      if(other.range_last()!=0)
         node=other.range_last();
      else 
         node=other.parent_;
      skip_children();
      ++(*this);
      }
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::post_order_iterator::operator++()
   {
   assert(node!=0);
   if(node->next_sibling==0) 
      node=node->parent;
   else {
      node=node->next_sibling;
      if(skip_current_children_) {
         skip_current_children_=false;
         }
      else {
         while(node->first_child)
            node=node->first_child;
         }
      }
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::post_order_iterator::operator--()
   {
   assert(node!=0);
   if(skip_current_children_ || node->last_child==0) {
      skip_current_children_=false;
      while(node->prev_sibling==0)
         node=node->parent;
      node=node->prev_sibling;
      }
   else {
      node=node->last_child;
      }
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::post_order_iterator::operator+=(unsigned int num)
   {
   while(num>0) {
      ++(*this);
      --num;
      }
   return (*this);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::post_order_iterator::operator-=(unsigned int num)
   {
   while(num>0) {
      --(*this);
      --num;
      }
   return (*this);
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::post_order_iterator::descend_all()
   {
   assert(node!=0);
   while(node->first_child)
      node=node->first_child;
   }



// Sibling iterator

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::sibling_iterator::sibling_iterator() 
   : iterator_base()
   {
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::sibling_iterator::sibling_iterator(tree_node *tn)
   : iterator_base(tn)
   {
   set_parent_();
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::sibling_iterator::sibling_iterator(const iterator_base& other)
   : iterator_base(other.node)
   {
   set_parent_();
   }

template <class T, class tree_node_allocator>
tree<T, tree_node_allocator>::sibling_iterator::sibling_iterator(const sibling_iterator& other)
   : iterator_base(other), parent_(other.parent_)
   {
   }

template <class T, class tree_node_allocator>
void tree<T, tree_node_allocator>::sibling_iterator::set_parent_()
   {
   parent_=0;
   if(node==0) return;
   if(node->parent!=0)
      parent_=node->parent;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::sibling_iterator::operator++()
   {
   if(node)
      node=node->next_sibling;
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::sibling_iterator::operator--()
   {
   if(node) node=node->prev_sibling;
   else {
      assert(parent_);
      node=parent_->last_child;
      }
   return *this;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::sibling_iterator::operator+=(unsigned int num)
   {
   while(num>0) {
      ++(*this);
      --num;
      }
   return (*this);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::iterator_base& tree<T, tree_node_allocator>::sibling_iterator::operator-=(unsigned int num)
   {
   while(num>0) {
      --(*this);
      --num;
      }
   return (*this);
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::tree_node *tree<T, tree_node_allocator>::sibling_iterator::range_first() const
   {
   tree_node *tmp=parent_->first_child;
   return tmp;
   }

template <class T, class tree_node_allocator>
typename tree<T, tree_node_allocator>::tree_node *tree<T, tree_node_allocator>::sibling_iterator::range_last() const
   {
   return parent_->last_child;
   }

#endif

// Local variables:
// default-tab-width: 3
// End:
