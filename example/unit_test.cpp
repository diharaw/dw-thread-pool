#include <iostream>
#include <chrono>

#include <thread_pool.hpp>
#include <Remotery.h>

#define TEST_CASE_1_NUM_TASKS 37
#define TEST_CASE_2_NUM_TASKS 29
#define TEST_CASE_3_NUM_TASKS 1000

#define TEST_CASE_4_ITERATIONS 5
#define TEST_CASE_5_ITERATIONS 5


int num_completed = 0;
std::mutex global_mutex;

void AnimationPreTransformUpdateTask(void* data)
{
    rmt_ScopedCPUSample(AnimationPreTransformUpdateTask, 0);

	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void TransformUpdateTask(void* data)
{
    rmt_ScopedCPUSample(TransformUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void PhysicsSyncTask(void* data)
{
    rmt_ScopedCPUSample(PhysicsSyncTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void AnimationPostTransformUpdateTask(void* data)
{
    rmt_ScopedCPUSample(AnimationPostTransformUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void AudioListenerUpdateTask(void* data)
{
    rmt_ScopedCPUSample(AudioListenerUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void AudioSourceUpdateTask(void* data)
{
    rmt_ScopedCPUSample(AudioSourceUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ParticleUpdateTask(void* data)
{
    rmt_ScopedCPUSample(ParticleUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ScriptUpdateTask(void* data)
{
    rmt_ScopedCPUSample(ScriptUpdateTask, 0);

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ecs_update(dw::ThreadPool& tp)
{
    dw::Task* animation_pre_transform_update_task = tp.allocate();
    dw::Task* transform_update_task               = tp.allocate();
    dw::Task* physics_sync_task                   = tp.allocate();
    dw::Task* animation_post_transform_update_task = tp.allocate();
    dw::Task* audio_listener_update_task           = tp.allocate();
    dw::Task* audio_source_update_task             = tp.allocate();
    dw::Task* particle_update_task                 = tp.allocate();
    dw::Task* script_update_task                   = tp.allocate();
    

    animation_pre_transform_update_task->function = AnimationPreTransformUpdateTask;
    transform_update_task->function                = TransformUpdateTask;
    physics_sync_task->function                    = PhysicsSyncTask;
    animation_post_transform_update_task->function = AnimationPostTransformUpdateTask;
    audio_listener_update_task->function          = AudioListenerUpdateTask;
    audio_source_update_task->function            = AudioSourceUpdateTask;
    particle_update_task->function                = ParticleUpdateTask;
    script_update_task->function                  = ScriptUpdateTask;

    // Continuations
	tp.define_continuation(animation_pre_transform_update_task, transform_update_task);
    tp.define_continuation(animation_pre_transform_update_task, physics_sync_task);

    tp.define_continuation(transform_update_task, animation_post_transform_update_task);
    tp.define_continuation(transform_update_task, audio_listener_update_task);
    tp.define_continuation(transform_update_task, audio_source_update_task);
    tp.define_continuation(transform_update_task, particle_update_task);

    tp.define_continuation(animation_post_transform_update_task, script_update_task);

    // Dependencies
    tp.define_dependency(script_update_task, audio_listener_update_task);
    tp.define_dependency(script_update_task, audio_source_update_task);
    tp.define_dependency(script_update_task, particle_update_task);

    tp.enqueue(animation_pre_transform_update_task);

    tp.wait_for_all();
}

int main()
{
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

	dw::ThreadPool thread_pool;

    for (int i = 0; i < 4; i++)
        ecs_update(thread_pool);

    int a;
    std::cin >> a;
    
    rmt_DestroyGlobalInstance(rmt);

	return 0;
}
