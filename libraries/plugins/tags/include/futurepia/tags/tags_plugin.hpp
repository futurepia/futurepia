#pragma once

#include <futurepia/chain/database.hpp>
#include <futurepia/chain/comment_object.hpp>

#include <futurepia/tags/tags_objects.hpp>

#include <fc/thread/future.hpp>
#include <fc/api.hpp>

namespace futurepia { namespace tags {

#define TAGS_PLUGIN_NAME "tags"


namespace detail { class tags_plugin_impl; }


/**
 *  This plugin will scan all changes to posts and/or their meta data and
 *
 */
class tags_plugin : public futurepia::app::plugin
{
   public:
      tags_plugin( application* app );
      virtual ~tags_plugin();

      std::string plugin_name()const override { return TAGS_PLUGIN_NAME; }
      virtual void plugin_set_program_options(
         boost::program_options::options_description& cli,
         boost::program_options::options_description& cfg) override;
      virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
      virtual void plugin_startup() override;

      friend class detail::tags_plugin_impl;
      std::unique_ptr<detail::tags_plugin_impl> my;
};

} } //futurepia::tag



