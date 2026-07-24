package com.gokrack.beatriceapp

import androidx.fragment.app.FragmentActivity
import androidx.viewpager2.adapter.FragmentStateAdapter

class MainPagerAdapter(activity: FragmentActivity) : FragmentStateAdapter(activity) {
    override fun getItemCount() = 5

    override fun createFragment(position: Int) = when (position) {
        0 -> SystemFragment()
        1 -> MainFragment()
        2 -> ParamsFragment()
        3 -> EffectorFragment()
        4 -> MorphingFragment()
        else -> SystemFragment()
    }
}
