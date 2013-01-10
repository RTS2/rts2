/*
 * Thread safe queue.
 * Copyright (C) 2012 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __RTS2_TSQUEUE__
#define __RTS2_TSQUEUE__

#include <queue>
#include <deque>
#include <pthread.h>

#include "error.h"

/**
 * Thread safe queue implementation. 
 */
template <class T, class Container = std::deque <T> > class TSQueue
{
	public:
		TSQueue ()
		{
			pthread_mutex_init (&m_mutex, NULL);
			pthread_cond_init (&m_condition, NULL);
		}

		~TSQueue ()
		{
			pthread_mutex_destroy (&m_mutex);
			pthread_cond_destroy (&m_condition);
		}

		void push (T value)
		{
			pthread_mutex_lock (&m_mutex);
			m_queue.push (value);
			if (!m_queue.empty ())
				pthread_cond_signal (&m_condition);
			pthread_mutex_unlock (&m_mutex);
		}

		T pop (bool block = false)
		{
			pthread_mutex_lock (&m_mutex);
			if (block)
			{
				while (m_queue.empty ())
					pthread_cond_wait (&m_condition, &m_mutex);
			}

			if (m_queue.empty ())
			{
				pthread_mutex_unlock (&m_mutex);
				throw rts2core::Error ("empty threads save queue");
			}

			T ret = m_queue.front ();
			m_queue.pop ();
	
			pthread_mutex_unlock (&m_mutex);
			return ret;
		}

		size_t size ()
		{
			pthread_mutex_lock (&m_mutex);
			size_t ret = m_queue.size ();
			pthread_mutex_unlock (&m_mutex);

			return ret;
		}

		bool empty ()
		{
			pthread_mutex_lock (&m_mutex);
			bool ret = m_queue.empty ();
			pthread_mutex_unlock (&m_mutex);

			return ret;
		}

	private:
		std::queue<T> m_queue;
		pthread_mutex_t m_mutex;
		pthread_cond_t m_condition;
};

#endif // ! __RTS2_TSQUEUE__
